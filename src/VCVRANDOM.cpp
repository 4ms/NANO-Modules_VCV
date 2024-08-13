#include "plugin.hpp"
#include <componentlibrary.hpp>
#include "NANOComponents.hpp"

#include "Resources/SynthTools/randomVoltage.hpp"

#define LED_SMOOTHING 0.00005f

#define RATE 0
#define PROB 1
#define RND 2
#define SHAPE 3
#define PARAMS 4
#define INPUTS 2
#define TRIG 0
#define IN 1

struct VCVRANDOM : Module
{
    float pots[PARAMS] = {0.0f, 0.0f, 0.0f, 0.0f};  
    float atvs[PARAMS] = {0.0f, 0.0f, 0.0f, 0.0f};
    float cvs[PARAMS] = {0.0f, 0.0f, 0.0f, 0.0f};
    float ins[INPUTS] = {0.0f, 0.0f};     
    bool sw = false;
    bool normCV = false;

    enum ParamIds
    {
        RATE_PARAM,
        PROB_PARAM,
        RND_PARAM,
        SHAPE_PARAM,
        OFFSET_SW,
        RATE_ATV,
        PROB_ATV,
        RND_ATV,
        SHAPE_ATV,
        NORM_CV,        
        NUM_PARAMS
    };
    enum InputIds
    {
        RATE_CV,
        PROB_CV,
        RND_CV,
        SHAPE_CV,
        TRIG_IN,
        VOLT_IN,
        NUM_INPUTS
    };
    enum OutputIds
    {
        TRIG_OUT,
        STEP_OUT,
        LIN_OUT,
        EXP_OUT,
        SMTH_OUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        RATE_LIGHT,
        PROB_LIGHT,
        RND_LIGHT,
        SHAPE_LIGHT,
		NUM_LIGHTS
	};

    enum ParamCurves{
	LIN_CURVE,
	EXP_CURVE,
	LOG_CURVE,
	CURVE_LAST,
    };

    RandomVoltage random;

    VCVRANDOM()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        // Config parameter ranges
        configParam(RATE_PARAM, 0.0f, 1.f, 0.5f, "RATE Parameter");
        configParam(PROB_PARAM, 0.0f, 1.f, 0.5f, "PROB Parameter");
        configParam(RND_PARAM, 0.0f, 1.f, 0.5f, "RND Parameter");
        configParam(SHAPE_PARAM, 0.0f, 1.f, 0.5f, "SHAPE Parameter");
        
        configParam(RATE_ATV, -1.0f, 1.f, 0.0f, "RATE Attenuverter");
        configParam(PROB_ATV, -1.0f, 1.f, 0.0f, "PROB Attenuverter");
        configParam(RND_ATV, -1.0f, 1.f, 0.0f, "RND Attenuverter");
        configParam(SHAPE_ATV, -1.0f, 1.f, 0.0f, "SHAPE Attenuverter");

        configSwitch(OFFSET_SW, 0.0f, 1.f, 0.0f, "OFFSET Switch", {"Bipolar", "Unipolar"});

        configInput(RATE_CV, "Rate CV");
        configInput(PROB_CV, "Prob CV");
        configInput(RND_CV, "Random CV");
        configInput(SHAPE_CV, "Shape CV");

        configInput(TRIG_IN, "Trigger");
        configInput(VOLT_IN, "Signal");

        configOutput(TRIG_OUT, "Trigger");

        configOutput(STEP_OUT, "Stepped");
        configOutput(LIN_OUT, "Linear");
        configOutput(EXP_OUT, "Exponential");
        configOutput(SMTH_OUT, "Smooth");

        random.init();
        random.setSampleRate(1.0f / (APP->engine->getSampleRate()));
        random.setOffset(0.0f);
    }

        // Parameter mapping tool that takes a 0-1 input and converts to the desired range & curve
    float setParameter(float input, float minValue, float maxValue, float curveFactor, enum ParamCurves curve){

        float outCurve = 0.0f;
        // Curve selection
        if(curve == LIN_CURVE){
            outCurve = input;
        } else if (curve == EXP_CURVE){
            outCurve = powf(input, curveFactor) / powf(1, curveFactor - 1);
        } else if (curve == LOG_CURVE){
            outCurve = powf(input * powf(1, curveFactor - 1), 1 / curveFactor);
        }
        // Range adaptation
        return (outCurve * (maxValue - minValue) + minValue);
    }

    void process(const ProcessArgs &args) override
    {   
        // Read voltage inputs
        for(uint32_t i = 0; i < PARAMS; i++){
            pots[i] = params[i].getValue();
            atvs[i] = params[RATE_ATV + i].getValue();
            // Check if the module is in CV normalization mode
            if(!normCV || inputs[RATE_CV + i].isConnected()){
                cvs[i] = inputs[i].getVoltage();
            } else {
                cvs[i] = random.getLinearOut();
            }
        }

        // Read specific inputs
        ins[TRIG] = inputs[TRIG_IN].getVoltage();
        ins[IN] = inputs[VOLT_IN].getVoltage();
        // Read offset switch state and convert to boolean
        sw = (bool)params[OFFSET_SW].getValue(); 

        // Calculate parameters with scaling and clamping
        float rate = clamp(pots[RATE_PARAM] + ((cvs[RATE_CV] * atvs[RATE_PARAM]) / 10.0f), 0.0f, 1.0f);
        rate = clamp(setParameter(rate, -5.0f, 10.0f, 1.15f, EXP_CURVE), -5.0f, 10.0f); // Clamp rate with setParameter adjustment
        float prob = clamp(pots[PROB_PARAM] + ((cvs[PROB_CV] * atvs[PROB_PARAM]) / 5.0f), 0.0f, 1.0f);
        float rnd  = clamp(pots[RND_PARAM] + ((cvs[RND_CV] * atvs[RND_PARAM]) / 5.0f), 0.0f, 1.0f);
        float shape = clamp(pots[SHAPE_PARAM] + ((cvs[SHAPE_CV] * atvs[SHAPE_PARAM]) / 5.0f), 0.0f, 1.0f);

        // Trigger input handling
        if(inputs[TRIG_IN].isConnected()){
            // Enable external trigger source
            random.setTriggerSource(true); 
            random.setTrigger(inputs[TRIG_IN].getVoltage());
        } else {
            // Disable external trigger source
            random.setTriggerSource(false);
        }

        // Voltage input handling
        if(inputs[VOLT_IN].isConnected()){
            // Enable external voltage sampling
            random.setSampleOrigin(true); 
            random.setVoltageSample(inputs[VOLT_IN].getVoltage());
        } else {
            // Disable external voltage sampling
            random.setSampleOrigin(false);
        }

        // Offset switch handling
        if(sw){
            random.setOffset(0.0f);
        } else {
            random.setOffset(5.0f);
        }

        // Set parameters in random object
        random.setRate(rate);
        random.setProbability(prob);
        random.setSpread(rnd);
        random.setShape(shape);

        // Process the random object
        random.process(); 

        // Update lights and outputs
        lights[RATE_LIGHT].setSmoothBrightness(random.getRateLight(), LED_SMOOTHING);
        lights[PROB_LIGHT].setSmoothBrightness(random.getProbLight(), LED_SMOOTHING);

        // Update lights based on switch state
        if(sw){
            lights[RND_LIGHT].setSmoothBrightness((random.getExponentialOut() - 5.0f), LED_SMOOTHING);
            lights[SHAPE_LIGHT].setSmoothBrightness((random.getExponentialOut() - 5.0f), LED_SMOOTHING);
        } else {
            lights[RND_LIGHT].setSmoothBrightness(random.getExponentialOut(), LED_SMOOTHING);
            lights[SHAPE_LIGHT].setSmoothBrightness(random.getExponentialOut(), LED_SMOOTHING);
        }

        // Set various outputs
        outputs[STEP_OUT].setVoltage(random.getSteppedOut());
        outputs[LIN_OUT].setVoltage(random.getLinearOut());
        outputs[EXP_OUT].setVoltage(random.getExponentialOut());
        outputs[SMTH_OUT].setVoltage(random.getSmoothOut());

        // Set trigger output voltage
        outputs[TRIG_OUT].setVoltage(random.getTriggerOut());
    }

    json_t *dataToJson() override { 
        json_t *rootJ = json_object();
        // Save the state of the normCV boolean
        json_object_set_new(rootJ, "normCV", json_boolean(normCV));
        return rootJ;
    }
    
    void dataFromJson(json_t *rootJ) override {
        // Restore the state of the normCV boolean
        json_t *normCVJ = json_object_get(rootJ, "normCV");
        if (normCVJ) {
            normCV = json_boolean_value(normCVJ);
        }
    }
};

struct VCVRANDOMWidget : ModuleWidget
{
    VCVRANDOMWidget(VCVRANDOM *module)
    {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/VCVRANDOM.svg")));

        addParam(createLightParamCentered<LEDSliderRed>(mm2px(Vec(7, 35)), module, VCVRANDOM::RATE_PARAM, VCVRANDOM::RATE_LIGHT));
        addParam(createLightParamCentered<LEDSliderRed>(mm2px(Vec(19, 35)), module, VCVRANDOM::PROB_PARAM, VCVRANDOM::PROB_LIGHT));
        addParam(createLightParamCentered<LEDSliderRed>(mm2px(Vec(31, 35)), module, VCVRANDOM::RND_PARAM, VCVRANDOM::RND_LIGHT));
        addParam(createLightParamCentered<LEDSliderRed>(mm2px(Vec(43, 35)), module, VCVRANDOM::SHAPE_PARAM, VCVRANDOM::SHAPE_LIGHT));

        addParam(createParamCentered<Trimpot>(mm2px(Vec(7, 67)), module, VCVRANDOM::RATE_ATV));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(19, 67)), module, VCVRANDOM::PROB_ATV));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(31, 67)), module, VCVRANDOM::RND_ATV));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(43, 67)), module, VCVRANDOM::SHAPE_ATV));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7,  82.25)), module, VCVRANDOM::RATE_CV));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(19,  82.25)), module, VCVRANDOM::PROB_CV));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(31,  82.25)), module, VCVRANDOM::RND_CV));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(43,  82.25)), module, VCVRANDOM::SHAPE_CV));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7,  95.25)), module, VCVRANDOM::TRIG_IN));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(19,  95.25)), module, VCVRANDOM::VOLT_IN));

        addParam(createParamCentered<NANOComponents::BarkSwitchSmall2P>(mm2px(Vec(31, 95.25)), module, VCVRANDOM::OFFSET_SW));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(43,  95.25)), module, VCVRANDOM::TRIG_OUT));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7,  108.25)), module, VCVRANDOM::STEP_OUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(19,  108.25)), module, VCVRANDOM::LIN_OUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(31,  108.25)), module, VCVRANDOM::EXP_OUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(43,  108.25)), module, VCVRANDOM::SMTH_OUT));

		addChild(createWidget<ScrewSilver>(Vec(14, 1.5)));
		//addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		//addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(120, 363.5)));

    }

 // Override the appendContextMenu method
    void appendContextMenu(Menu *menu) override {
        ModuleWidget::appendContextMenu(menu);
        VCVRANDOM* myModule = dynamic_cast<VCVRANDOM*>(module);
        assert(myModule);

        // Add menu items
        menu->addChild(createMenuLabel("Normalize OUT to CV INs"));

        struct HiddenOptionMenuItem : MenuItem {
            VCVRANDOM *module;
            HiddenOptionMenuItem(VCVRANDOM *module) : module(module) {
                this->text = module->normCV ? "Turn OFF" : "Turn ON";
            }

            void onAction(const event::Action &e) override {
                module->normCV = !module->normCV;
                this->text = module->normCV ? "Turn OFF" : "Turn ON";
            }
        };

        menu->addChild(new HiddenOptionMenuItem(myModule));
    }
};

Model *modelVCVRANDOM = createModel<VCVRANDOM, VCVRANDOMWidget>("VCVRANDOM");