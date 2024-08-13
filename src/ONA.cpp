#include "plugin.hpp"
#include <componentlibrary.hpp>
#include "NANOComponents.hpp"

const float BASE_SCALE = 16.36f;
const float LFO_SCALE = 0.255625f;

//Struct to handle potentiometer variables
struct potentiometers
{
    float octave;
    float fine;
    float fm;
    float pw;
};

//Struct to handle input variables
struct ins
{
    float voct;
    float fm;
    float sync;
    float pw;
};

//Struct to handle oscillator variables
struct oscillator
{
    float pitch;
    float frequency;
    float octave;
    float waveform;
    float phase;
    float phaseIncrement;
    float delta;
    float max;
    float T;
    float output;
    float mix;
    float outputMix;
    uint32_t mode;
};

//Main VCO phase handler
struct oscillator phasesHandler(oscillator p)
{

    //Main phase handler
    p.phaseIncrement = p.frequency * p.delta;
    //Prevent phase overflow
    p.phase += p.phaseIncrement;
    //Reset control
    if (p.phase >= p.max)
    {
        p.phase -= p.max;
    }
    //Phase ratio update
    p.T = p.phase / p.max;

    return p;
}

struct triggers
{
    float inputValue;
    float clockValue;
    float externalClock;
    float triggerValue;
    float triggerPastValue;
    float gateValue;
    float deltaTrigger;
    float risingEdge;
};

struct triggers syncTriggerHandler(triggers t)
{

    //Trigger & Gate activated at rising edge
    if (t.inputValue > 0.7f && t.triggerPastValue < 0.7f)
    {

        t.triggerValue = 1.0f;
        t.gateValue = 1.0f;
        t.risingEdge = 1.0f;

        //Trigger & Gate deactivated at falling edge
    }
    else if (t.inputValue < 0.7f && t.triggerPastValue > 0.7f)
    {

        t.triggerValue = 0.0f;
        t.gateValue = 0.0f;

        //Trigger deactivated if triggered before
    }
    else if (t.inputValue > 0.7f && t.triggerPastValue > 0.7f)
    {

        t.triggerValue = 0.0f;
        t.risingEdge = 0.0f;
    }

    //State controller
    t.triggerPastValue = t.inputValue;

    return t;
}

//Just some maths values
struct maths
{
    float PI, twoPI;
    float intPI, twoIntPI;
};

struct ONA : Module
{
    float deltaTime = 1.0f / 44100.0f;
    float morph1 = 0.0f;
    float morph2 = 0.0f;
    float phaseChange = 0.0f;

    enum ParamIds
    {
        OCT_PARAM,
        FINE_PARAM,
        FM_PARAM,
        PW_PARAM,
        FM_SW_PARAM,
        MODE_PARAM,
        NUM_PARAMS
    };
    enum InputIds
    {
        OCT_INPUT,
        FM_INPUT,
        SYNC_INPUT,
        PW_INPUT,
        NUM_INPUTS
    };
    enum OutputIds
    {
        SINE_OUTPUT,
        TRIANGLE_OUTPUT,
        SAW_OUTPUT,
        PULSE_OUTPUT,
        MORPH1_OUTPUT,
        MORPH2_OUTPUT,
        SUB1_OUTPUT,
        SUB2_OUTPUT,
        NUM_OUTPUTS
    };

    //Internal structs declaration
    potentiometers potsValue;
    maths mathsValue;
    ins inputsValue;
    //3 VCOs required, main and both subs
    oscillator mainVCO;
    oscillator sub1VCO;
    oscillator sub2VCO;
    //Sync trigger handler
    triggers syncTrigger;

    ONA()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);

        configParam(OCT_PARAM, 0, 7, 3, "Octave");
        configParam(FINE_PARAM, -7.0f, 7.0f, 0.0f, "Fine frequency");

        configParam(FM_PARAM, 0.f, 1.f, 0.5f, "FM attenuverter");
        configParam(PW_PARAM, 0.f, 1.f, 0.5f, "Pulse Width Attenuverter");

        configSwitch(FM_SW_PARAM, 0.f, 1.f, 1.0f, "FM Mode",  {"Exponential", "Linear"});
        configSwitch(MODE_PARAM, 0.f, 1.f, 1.0f, "Oscillator Mode",  {"LFO", "VCO"});

        configInput(FM_INPUT, "FM");
        configInput(OCT_INPUT, "V/OCT");
        configInput(SYNC_INPUT, "SYNC");
        configInput(PW_INPUT, "PWM");

        configOutput(MORPH1_OUTPUT, "Morph 1");
        configOutput(MORPH2_OUTPUT, "Morph 2");
        configOutput(SUB1_OUTPUT, "Sub 1");
        configOutput(SUB2_OUTPUT, "Sub 2");

        configOutput(SINE_OUTPUT, "Sine");
        configOutput(TRIANGLE_OUTPUT, "Triangle");
        configOutput(SAW_OUTPUT, "Saw");
        configOutput(PULSE_OUTPUT, "Pulse");

        //Init values
        mathsValue.PI = 3.141592653589793f;
        mathsValue.twoPI = 2.0f * mathsValue.PI;

        mainVCO.max = mathsValue.twoPI;
        mainVCO.phase = mathsValue.twoPI;
        mainVCO.frequency = 110.0f;

        sub1VCO.max = mathsValue.twoPI;
        sub1VCO.phase = mathsValue.twoPI;
        sub1VCO.frequency = 110.0f;

        sub2VCO.max = mathsValue.twoPI;
        sub2VCO.phase = mathsValue.twoPI;
        sub2VCO.frequency = 110.0f;

        syncTrigger.inputValue = 0.0f;
        syncTrigger.clockValue = 0.0f;
        syncTrigger.externalClock = 0.0f;
        syncTrigger.triggerValue = 0.0f;
        syncTrigger.triggerPastValue = 0.0f;
        syncTrigger.gateValue = 0.0f;
        syncTrigger.deltaTrigger = 12.0f;
        syncTrigger.risingEdge = 0.0f;
    }

    //PolyBLEP anti-aliasing function
    double PolyBLEP(double t, double phaseIncrement)
    {

        double dt = phaseIncrement / mathsValue.twoPI;
        // 0 <= t < 1
        if (t < dt)
        {
            t /= dt;
            return t + t - t * t - 1.0;
        } // -1 < t < 0
        else if (t > 1.0 - dt)
        {
            t = (t - 1.0) / dt;
            return t * t + t + t + 1.0;
        }
        // 0 otherwise
        else
            return 0.0;
    }

    //Sine wave shaper
    double sineWave(float phase)
    {
        //Compute the SINE output
        return -10.0f * std::cos(phase);
    }

    //Triangle wave shaper
    double triangleWave(float phase)
    {
        //Compute the TRIANGLE output
        double tri = -1.0 + (2.0 * phase / mathsValue.twoPI);
        tri = 2.0 * (fabs(tri) - 0.5);
        return -tri * 10.0f;
    }

    //Saw (BLEP) wave shaper
    double sawWave(float phase, double T, double phaseIncrement)
    {
        //Compute the SAW output
        double saw = (2.0 * phase / mathsValue.twoPI) - 1.0;
        saw -= PolyBLEP(T, phaseIncrement);
        return saw * 10.0f;
    }

    //Pulse (BLEP) wave shaper
    double pulseWave(float phase, float duty, double T, double phaseIncrement)
    {
        //Compute the PULSE output
        double pul = 0.0f;
        if (phase < duty * mathsValue.twoPI)
        {
            pul = 1.0;
        }
        else
        {
            pul = -1.0;
        }
        pul += PolyBLEP(T, phaseIncrement);
        pul -= PolyBLEP(fmod(T + (1.0f - duty), 1.0), phaseIncrement);
        return pul * 10.0f;
    }

    //Parameters, pots and inputs monitoring
    void Values_Handler(void)
    {
        //POTENTIOMETER READINGS
        //Frequency related pots
        potsValue.octave = params[OCT_PARAM].getValue();
        potsValue.fine = params[FINE_PARAM].getValue();

        //CV Related pots
        potsValue.fm = params[FM_PARAM].getValue();
        potsValue.pw = params[PW_PARAM].getValue();

        //INPUTS READINGS
        //V.Oct related CV ins
        inputsValue.voct = inputs[OCT_INPUT].getVoltage();
        //Freq related CV ins
        inputsValue.fm = inputs[FM_INPUT].getVoltage();
        inputsValue.sync = inputs[SYNC_INPUT].getVoltage();
        //Shape related CV ins
        inputsValue.pw = inputs[PW_INPUT].getVoltage();
    }

    //Deduce variables from the parameters pots and inputs
    void Variable_Handler(void)
    {
        //VARIABLE COMPUTING

        //Sync handler
        syncTrigger.inputValue = inputsValue.sync;
        syncTrigger = syncTriggerHandler(syncTrigger);

        if (syncTrigger.risingEdge >= 1.0f)
        {
            mainVCO.phase = 0.0f;
            sub1VCO.phase = 0.0f;
            sub2VCO.phase = 0.0f;
        }

        //VCO frequency
        //Octave switch
        mainVCO.pitch = (uint32_t)potsValue.octave;
        //Sum fine frequency adjust
        mainVCO.pitch += potsValue.fine * 0.08333333f;
        //Sum V/OCT input voltage
        mainVCO.pitch += inputsValue.voct;

        //Linear or exponential FM
        if (params[FM_SW_PARAM].getValue())
        {
            //Linear FM is basically phase modulation
            mainVCO.phase += inputsValue.fm * potsValue.fm * 0.01f;
            sub1VCO.phase += inputsValue.fm * potsValue.fm * 0.005f;
            sub2VCO.phase += inputsValue.fm * potsValue.fm * 0.0025f;
            //Just don't go out of the limits
            mainVCO.phase = clamp(mainVCO.phase, 0.0f, mathsValue.twoPI);
            sub1VCO.phase = clamp(sub1VCO.phase, 0.0f, mathsValue.twoPI);
            sub2VCO.phase = clamp(sub2VCO.phase, 0.0f, mathsValue.twoPI);

            //Notify phase change
            phaseChange = 1.0f;
        }
        else
        {
            //Exponential FM
            mainVCO.pitch += inputsValue.fm * potsValue.fm;
            //If we come from linear FM, reset phase;
            if (phaseChange)
            {
                mainVCO.phase = 0.0f;
                sub1VCO.phase = 0.0f;
                sub2VCO.phase = 0.0f;
                phaseChange = 0.0f;
            }
        }

        //VCO or LFO mode
        if (params[MODE_PARAM].getValue())
        {
            //Frequency of the VCO is calculated with the power of the previous voltage sum
            mainVCO.frequency = clamp((BASE_SCALE * mathsValue.twoPI * std::pow(2, mainVCO.pitch)), 0.01f, 22100.0f * mathsValue.twoPI);
        }
        else
        {
            //Frequency of the LFO is calculated with the power of the previous voltage sum
            mainVCO.frequency = clamp((LFO_SCALE / 64.0f * mathsValue.twoPI * std::pow(3, mainVCO.pitch)), 0.01f, 22100.0f * mathsValue.twoPI);
        }

        //Sub frequency update
        sub1VCO.frequency = mainVCO.frequency / 2.0f;
        sub2VCO.frequency = mainVCO.frequency / 4.0f;

        //PWM control + attenuator
        if (!inputs[PW_INPUT].getVoltage())
        {
            //If there's no PWM input connected, get the value from the PWM pot
            //mainVCO.waveform = clamp(potsValue.pw, 0.01f, 0.99f);
            mainVCO.waveform = clamp(potsValue.pw, 0.01f, 1.0f);
        }
        else
        {
            //If there's PWM input connected, use the PWM pot as attenuator
            mainVCO.waveform = clamp((inputsValue.pw * potsValue.pw * 0.2f), 0.01f, 1.0f);
        }
    }

    void process(const ProcessArgs &args) override
    {
        //Sample rate update
        deltaTime = args.sampleTime;
        mainVCO.delta = deltaTime;
        sub1VCO.delta = deltaTime;
        sub2VCO.delta = deltaTime;

        //Parameters & CVs monitoring
        Values_Handler();

        //Variable writing and handling
        Variable_Handler();

        //Phase hander
        mainVCO = phasesHandler(mainVCO);
        sub1VCO = phasesHandler(sub1VCO);
        sub2VCO = phasesHandler(sub2VCO);

        //Just output value writing
        outputs[SINE_OUTPUT].setVoltage(sineWave(mainVCO.phase) * 0.5f);
        outputs[TRIANGLE_OUTPUT].setVoltage(triangleWave(mainVCO.phase) * 0.5f);
        outputs[SAW_OUTPUT].setVoltage(sawWave(mainVCO.phase, mainVCO.T, mainVCO.phaseIncrement) * 0.5f);
        outputs[PULSE_OUTPUT].setVoltage(pulseWave(mainVCO.phase, mainVCO.waveform, mainVCO.T, mainVCO.phaseIncrement) * 0.5f);

        //Complex waveform generator, acts like an analog switch
        //If the pulse wave is positive, open the switch
        if (pulseWave(mainVCO.phase, mainVCO.waveform, mainVCO.T, mainVCO.phaseIncrement) > 0.0f)
        {
            //Add triangle wave to the morph1 output, only when pulse wave is active
            morph1 = +triangleWave(mainVCO.phase);
            //Add saw wave to the morph2 output, only when pulse wave is active
            morph2 = +sawWave(mainVCO.phase, mainVCO.T, mainVCO.phaseIncrement);
        }
        else
        {
            //If not positive, don't add anything
            morph1 = 0;
            morph2 = 0;
        }

        //Set morph output values
        outputs[MORPH1_OUTPUT].setVoltage((-triangleWave(mainVCO.phase) + morph1) * 0.5f);
        outputs[MORPH2_OUTPUT].setVoltage((-triangleWave(mainVCO.phase) + morph2) * 0.5f);

        //Set sub output values
        outputs[SUB1_OUTPUT].setVoltage(pulseWave(sub1VCO.phase, 0.5f, sub1VCO.T, sub1VCO.phaseIncrement) * 0.5f);
        outputs[SUB2_OUTPUT].setVoltage(pulseWave(sub2VCO.phase, 0.5f, sub2VCO.T, sub2VCO.phaseIncrement) * 0.5f);
    }
};

struct ONAWidget : ModuleWidget
{
    ONAWidget(ONA *module)
    {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ONA.svg")));

        addChild(createWidget<ScrewSilver>(Vec(14, 1.5)));
        //addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        //addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(90, 363.5)));

        addParam(createParamCentered<NANOComponents::NANOBlackKnobSwitch>(mm2px(Vec(20, 18.5)), module, ONA::OCT_PARAM));
        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(20, 49)), module, ONA::FINE_PARAM));

        addParam(createParamCentered<Trimpot>(mm2px(Vec(7.65, 66.75)), module, ONA::FM_PARAM));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(32.5, 66.75)), module, ONA::PW_PARAM));

        addParam(createParamCentered<NANOComponents::BarkSwitchSmall2P>(mm2px(Vec(7.5, 37.0)), module, ONA::FM_SW_PARAM));
        addParam(createParamCentered<NANOComponents::BarkSwitchSmall2P>(mm2px(Vec(32.5, 37.0)), module, ONA::MODE_PARAM));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5, 82.25)), module, ONA::FM_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15, 82.25)), module, ONA::OCT_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25, 82.25)), module, ONA::SYNC_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(35, 82.25)), module, ONA::PW_INPUT));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5, 95.25)), module, ONA::MORPH1_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15, 95.25)), module, ONA::MORPH2_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(25, 95.25)), module, ONA::SUB1_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(35, 95.25)), module, ONA::SUB2_OUTPUT));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5, 108.25)), module, ONA::SINE_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15, 108.25)), module, ONA::TRIANGLE_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(25, 108.25)), module, ONA::SAW_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(35, 108.25)), module, ONA::PULSE_OUTPUT));
    }
};

Model *modelONA = createModel<ONA, ONAWidget>("ONA");