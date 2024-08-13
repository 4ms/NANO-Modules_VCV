#include "plugin.hpp"
#include <componentlibrary.hpp>
#include "NANOComponents.hpp"

#define LED_SMOOTHING 0.00005f

struct STMAR : Module
{   
    float l1_input, r1_input;
    float l2_input, r2_input;
    float l3_input, r3_input;
    float cv1_input, cv2_input, cv3_input;

    enum ParamIds
    {
        POT1_PARAM,
        POT2_PARAM,
        POT3_PARAM,
        POTM_PARAM,
        MUTE1_PARAM,
        MUTE2_PARAM,
        MUTE3_PARAM,
        NUM_PARAMS
    };
    enum InputIds
    {
        L1_INPUT,
        R1_INPUT,
        L2_INPUT,
        R2_INPUT,
        L3_INPUT,
        R3_INPUT,
        CV1_INPUT,
        CV2_INPUT,
        CV3_INPUT,                
        NUM_INPUTS
    };
    enum OutputIds
    {
        L_OUTPUT,
        R_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        L_LIGHT,
        R_LIGHT,
        CLIPL_LIGHT,
        CLIPR_LIGHT,
		NUM_LIGHTS
	};

    STMAR()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        // Config parameter ranges
        configParam(POT1_PARAM, 0.f, 1.f, 0.75f, "Channel 1 gain amount");
        configParam(POT2_PARAM, 0.f, 1.f, 0.75f, "Channel 2 gain amount");
        configParam(POT3_PARAM, 0.f, 1.f, 0.75f, "Channel 3 gain amount");
        configParam(POTM_PARAM, 0.f, 1.f, 1.0f, "Master channel gain amount");

        configSwitch(MUTE1_PARAM, 0.0f, 1.f, 1.0f, "Mute switch 1", {"ON", "OFF"});
        configSwitch(MUTE2_PARAM, 0.0f, 1.f, 1.0f, "Mute switch 2", {"ON", "OFF"});
        configSwitch(MUTE3_PARAM, 0.0f, 1.f, 1.0f, "Mute switch 3", {"ON", "OFF"});

        configInput(L1_INPUT, "L1");
        configInput(R1_INPUT, "R1");
        configInput(CV1_INPUT, "CV1");

        configInput(L2_INPUT, "L2");
        configInput(R2_INPUT, "R2");
        configInput(CV2_INPUT, "CV2");

        configInput(L3_INPUT, "L3");
        configInput(R3_INPUT, "R3");
        configInput(CV3_INPUT, "CV3");

        configOutput(L_OUTPUT, "L");
        configOutput(R_OUTPUT, "R");
    }

    void process(const ProcessArgs &args) override
    {   
        // Read voltage inputs
        l1_input = inputs[L1_INPUT].getVoltage();
        l2_input = inputs[L2_INPUT].getVoltage();
        l3_input = inputs[L3_INPUT].getVoltage();

        // Do input signals normalization if required and save the input values
        if(inputs[R1_INPUT].isConnected()){
            r1_input = inputs[R1_INPUT].getVoltage();
        } else {
            r1_input = l1_input;
        }

        if(inputs[R2_INPUT].isConnected()){
            r2_input = inputs[R2_INPUT].getVoltage();
        } else {
            r2_input = l2_input;
        }

        if(inputs[R3_INPUT].isConnected()){
            r3_input = inputs[R3_INPUT].getVoltage();
        } else {
            r3_input = l3_input;
        }

        // Do CV signals normalization if required and save the CV values
        if(inputs[CV1_INPUT].isConnected()){
            cv1_input = clamp(inputs[CV1_INPUT].getVoltage(), 0.0f, 10.0f);
        } else {
            cv1_input = 5.0f;
        }

        if(inputs[CV2_INPUT].isConnected()){
            cv2_input = clamp(inputs[CV2_INPUT].getVoltage(), 0.0f, 10.0f);
        } else {
            cv2_input = 5.0f;
        }

        if(inputs[CV3_INPUT].isConnected()){
            cv3_input = clamp(inputs[CV3_INPUT].getVoltage(), 0.0f, 10.0f);
        } else {
            cv3_input = 5.0f;
        }

        // Get channel gains
        float gain1 = (params[POT1_PARAM].getValue() * cv1_input / 5.0f) * params[MUTE1_PARAM].getValue();
        float gain2 = (params[POT2_PARAM].getValue() * cv2_input / 5.0f) * params[MUTE2_PARAM].getValue();
        float gain3 = (params[POT3_PARAM].getValue() * cv3_input / 5.0f) * params[MUTE3_PARAM].getValue();
        float gainm = params[POTM_PARAM].getValue();

        // Compute mix
        float mix_l = (l1_input * gain1) + (l2_input * gain2) + (l3_input * gain3);
        float mix_r = (r1_input * gain1) + (r2_input * gain2) + (r3_input * gain3);

        // Calculate final mix and clamp it to avoid clipping and failure
        float out_l = clamp(mix_l * gainm, -11.0f, 11.0f);
        float out_r = clamp(mix_r * gainm, -11.0f, 11.0f);

        // Write the output voltages
        outputs[L_OUTPUT].setVoltage(out_l);
        outputs[R_OUTPUT].setVoltage(out_r);

        // Set the LEDs brightness depending on output signal
        lights[L_LIGHT].setSmoothBrightness(mix_l / 5.0, LED_SMOOTHING);
        lights[R_LIGHT].setSmoothBrightness(mix_r / 5.0, LED_SMOOTHING);

        // Use a comparator for the clip leds
        if(abs(mix_l) > 10.0f){
            lights[CLIPL_LIGHT].setSmoothBrightness(1.0f, LED_SMOOTHING);
        } else {
            lights[CLIPL_LIGHT].setSmoothBrightness(0.0f, LED_SMOOTHING);
        }

        if(abs(mix_r) > 10.0f){
            lights[CLIPR_LIGHT].setSmoothBrightness(1.0f, LED_SMOOTHING);
        } else {
            lights[CLIPR_LIGHT].setSmoothBrightness(0.0f, LED_SMOOTHING);
        }

    }
};

struct STMARWidget : ModuleWidget
{
    STMARWidget(STMAR *module)
    {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/STMAR.svg")));

		addChild(createWidget<ScrewSilver>(Vec(14, 1.5)));
		//addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		//addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(90, 363.5)));

        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(30.0, 20.25)), module, STMAR::POT1_PARAM));
        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(30.0, 47.75)), module, STMAR::POT2_PARAM));
        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(30.0, 75.40)), module, STMAR::POT3_PARAM));
        addParam(createParamCentered<Davies1900hBlackKnob>(mm2px(Vec(30.0, 103.0)), module, STMAR::POTM_PARAM));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5,  14.50)), module, STMAR::L1_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15, 14.50)), module, STMAR::R1_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5 , 27.50)), module, STMAR::CV1_INPUT));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5,  42.00)), module, STMAR::L2_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15, 42.00)), module, STMAR::R2_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5 , 55.00)), module, STMAR::CV2_INPUT));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5,  69.25)), module, STMAR::L3_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15, 69.25)), module, STMAR::R3_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5 , 82.25)), module, STMAR::CV3_INPUT));

        addParam(createParamCentered<NANOComponents::BarkSwitchSmall2P>(mm2px(Vec(15.0, 27.50)), module, STMAR::MUTE1_PARAM));
        addParam(createParamCentered<NANOComponents::BarkSwitchSmall2P>(mm2px(Vec(15.0, 54.75)), module, STMAR::MUTE2_PARAM));
        addParam(createParamCentered<NANOComponents::BarkSwitchSmall2P>(mm2px(Vec(15.0, 82.25)), module, STMAR::MUTE3_PARAM));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5, 108.25)), module, STMAR::L_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15, 108.25)), module, STMAR::R_OUTPUT));

        addChild(createLightCentered<MediumLight<WhiteLight>>(mm2px(Vec(4.00, 99.5)), module, STMAR::L_LIGHT));
        addChild(createLightCentered<MediumLight<WhiteLight>>(mm2px(Vec(16.0, 99.5)), module, STMAR::R_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(4.00, 95.0)), module, STMAR::CLIPL_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(16.0, 95.0)), module, STMAR::CLIPR_LIGHT));
    }
};

Model *modelSTMAR = createModel<STMAR, STMARWidget>("STMAR");