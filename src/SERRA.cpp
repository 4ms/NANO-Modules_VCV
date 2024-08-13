// Include necessary headers for the plugin framework, standard component library, and custom components.
#include "plugin.hpp"
#include <componentlibrary.hpp>
#include "NANOComponents.hpp"

// Include resources for additional envelope and shaping functionalities.
#include "Resources/SynthTools/envelope.hpp"
#include "Resources/SynthTools/shaper.hpp"

// Define constants for envelope shaping and output gains.
#define SLOW 20.0f
#define FAST 0.5f
#define MIN 0.001f
#define STEEPNESS 0.5f

#define ENV_GAIN 10.0f
#define EOC_GAIN 5.0f
#define CV_GAIN 7.5f
#define STEEPNESS 0.5f

// Define the SERRA module, inheriting from the base Module class.
struct SERRA : Module
{
    // Enumeration for identifying module parameters.
    enum ParamIds
    {
        ATTVER_PARAM,     // Signal attenuverter.
        OFFSET_PARAM,     // Signal offset.
        ATTACK_PARAM,     // Attack time.
        DECAY_PARAM,      // Decay time.
        SUSTAIN_PARAM,    // Sustain level.
        RELEASE_PARAM,    // Release time.
        GATE_SW_PARAM,    // Gate switch.
        SPEED_SW_PARAM,   // Speed switch (affects time constants).
        NORM_SW_PARAM,    // CV normalization switch.
        NUM_PARAMS        // Total number of parameters.
    };

    // Enumeration for identifying module inputs.
    enum InputIds
    {
        GATE_INPUT,       // Gate input.
        TRIG_INPUT,       // Trigger input.
        SIG_INPUT,        // Signal input for processing.
        A_CV_INPUT,       // CV input for Attack time modulation.
        D_CV_INPUT,       // CV input for Decay time modulation.
        S_CV_INPUT,       // CV input for Sustain level modulation.
        R_CV_INPUT,       // CV input for Release time modulation.
        NUM_INPUTS        // Total number of inputs.
    };

    // Enumeration for identifying module outputs.
    enum OutputIds
    {
        ENV_OUTPUT,       // Envelope output.
        EOC_OUTPUT,       // End of Cycle (EOC) output.
        OUT_OUTPUT,       // Processed signal output.
        NUM_OUTPUTS       // Total number of outputs.
    };

    // Enumeration for identifying module lights.
    enum LightIds {
        GATE_SW_LIGHT,    // Light for the gate switch.
        ENV_LIGHT,        // Light indicating envelope activity.
        EOC_LIGHT,        // Light indicating end of cycle.
        ENUMS(OUT_LIGHT, 2), // Lights indicating output signal level.
		NUM_LIGHTS        // Total number of lights.
	};

    // ADSR envelope generator object.
    ADSREnvelope ENV;

    // Constructor for initializing module parameters and envelope generator.
    SERRA()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        // Configuration of module parameters with default values and descriptions.
        configParam(ATTVER_PARAM, -1.f, 1.f, 1.0f, "Signal attenuverter");
        configParam(OFFSET_PARAM, -5.f, 5.f, 0.0f, "Signal offset");
        // Parameters for envelope stages.
        configParam(ATTACK_PARAM, 0.f, 1.f, 0.5f, "Attack time");
        configParam(DECAY_PARAM, 0.f, 1.f, 0.5f, "Decay time");
        configParam(SUSTAIN_PARAM, 0.f, 1.f, 0.5f, "Sustain level");
        configParam(RELEASE_PARAM, 0.f, 1.f, 0.5f, "Release time");
        // Additional functional switches.
        configSwitch(GATE_SW_PARAM, 0.f, 5.f, 0.0f, "Gate switch");
        configSwitch(SPEED_SW_PARAM, 0.0f, 1.f, 0.0f, "Speed switch", {"FAST", "SLOW"});
        configSwitch(NORM_SW_PARAM, 0.0f, 1.f, 0.0f, "CV normalization switch", {"INDEP", "NORM"});

        configInput(A_CV_INPUT, "Attack CV");
        configInput(D_CV_INPUT, "Decay CV");
        configInput(S_CV_INPUT, "Sustain CV");
        configInput(R_CV_INPUT, "Release CV");
        configInput(SIG_INPUT, "Signal");

        configInput(GATE_INPUT, "Gate");
        configInput(TRIG_INPUT, "Trigger");

        configOutput(ENV_OUTPUT, "ENV");
        configOutput(EOC_OUTPUT, "EOC");
        configOutput(OUT_OUTPUT, "OUT");

        // Initialize the envelope generator with the sample rate.
        ENV.init(APP->engine->getSampleRate());
        // Configure the envelope generator's characteristics.
        ENV.setOutputLevel(ENV_GAIN);
        ENV.setAttackShape(1.0f);
        ENV.setDecayReleaseShape(0.0f);
    }

    // Process function to handle real-time module operations.
    void process(const ProcessArgs &args) override
    {   
        // Check if CV normalization is activated.
        bool normActive = params[NORM_SW_PARAM].getValue();
        // Determine gate status based on the gate switch and input.
        float gateInput = clamp((params[GATE_SW_PARAM].getValue() + inputs[GATE_INPUT].getVoltage()), 0.0f, 1.0f);

        float timeFactor;
                
        // Determine time factor based on the speed switch.
        if(!params[SPEED_SW_PARAM].getValue()){
            timeFactor = 1.0f; // Normal speed.
        } else {
            timeFactor = 10.0f; // Increased speed for envelope stages.
        }

        float attack, decay, sustain, release;
        // Process CV inputs for envelope stages, considering normalization.
        if(normActive){
            // Normalize decay, sustain, and release to the attack CV input if normalization is active.
            attack  = inputs[A_CV_INPUT].getVoltage();
            decay   = attack;
            release = attack;
        } else {
            // Use individual CV inputs for each envelope stage.
            attack  = inputs[A_CV_INPUT].getVoltage();
            decay   = inputs[D_CV_INPUT].getVoltage();
            release = inputs[R_CV_INPUT].getVoltage();
        }

        sustain = inputs[S_CV_INPUT].getVoltage();

        // Calculate time constants for each stage using shaped curves.
        float attackFactor   = Shaper::shapeCurve(params[ATTACK_PARAM ].getValue() + attack  / CV_GAIN, MIN, FAST, 0.0f, STEEPNESS);
        float decayFactor    = Shaper::shapeCurve(params[DECAY_PARAM  ].getValue() + decay   / CV_GAIN, MIN, SLOW, 0.0f, STEEPNESS);
        float releaseFactor  = Shaper::shapeCurve(params[RELEASE_PARAM].getValue() + release / CV_GAIN, MIN, SLOW, 0.0f, STEEPNESS);
        float sustainFactor  = params[SUSTAIN_PARAM].getValue() + sustain / CV_GAIN;

        // Apply calculated factors to the envelope generator.
        ENV.setAttack(attackFactor * timeFactor);
        ENV.setDecay(decayFactor * timeFactor);
        ENV.setSustain(sustainFactor);
        ENV.setRelease(releaseFactor * timeFactor);

        // Retrigger envelope generator if trigger input is active.
        ENV.retrigger(inputs[TRIG_INPUT].getVoltage());

        // Process the envelope with the current gate status.
        ENV.process((bool)gateInput);
        float envelopeOut = ENV.getCurveOutput();
        // Apply attenuverter and offset to the envelope output.
        float envelopeMod = ((envelopeOut * params[ATTVER_PARAM].getValue()) + params[OFFSET_PARAM].getValue());
        // Get end of cycle status.
        float envelopeEoc = ENV.getEndOfCycle();

        float signalOut;

        // Process signal input with or without normalization.
        if (inputs[SIG_INPUT].isConnected()){
            signalOut = (inputs[SIG_INPUT].getVoltage() + params[OFFSET_PARAM].getValue()) * params[ATTVER_PARAM].getValue();
        } else {
            signalOut = envelopeMod;
        }

        // Output the processed signals.
        outputs[ENV_OUTPUT].setVoltage(envelopeOut);
        outputs[EOC_OUTPUT].setVoltage(envelopeEoc * EOC_GAIN);
        outputs[OUT_OUTPUT].setVoltage(signalOut);
        
        // Update module lights based on the current states.
        lights[GATE_SW_LIGHT].setSmoothBrightness(gateInput, 0.01f);
        lights[ENV_LIGHT].setSmoothBrightness(envelopeOut, 0.01f);
        lights[EOC_LIGHT].setSmoothBrightness(envelopeEoc, 0.01f);
        lights[OUT_LIGHT + 0].setSmoothBrightness(fmaxf(0.0, signalOut / 5.0), 0.01f);
		lights[OUT_LIGHT + 1].setSmoothBrightness(fmaxf(0.0, -signalOut / 5.0), 0.01f);
    }
};

struct SERRAWidget : ModuleWidget
{
    SERRAWidget(SERRA *module)
    {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SERRA.svg")));

        addChild(createWidget<ScrewSilver>(Vec(14, 1.5)));
        //addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        //addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(120, 363.5)));

        addParam(createParamCentered<PB61303>(mm2px(Vec(25.0, 36.25)), module, SERRA::GATE_SW_PARAM));
        addChild(createLightCentered<PB61303Light<WhiteLight>>(mm2px(Vec(25.0, 36.25)), module, SERRA::GATE_SW_LIGHT));

        addParam(createParamCentered<NANOComponents::BarkSwitchSmall2P>(mm2px(Vec(9.0, 36.25)), module, SERRA::NORM_SW_PARAM));
        addParam(createParamCentered<NANOComponents::BarkSwitchSmall2P>(mm2px(Vec(41.0, 36.25)), module, SERRA::SPEED_SW_PARAM));

        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(38.0, 15.25)), module, SERRA::OFFSET_PARAM));
        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(12.0, 15.25)), module, SERRA::ATTVER_PARAM));

        addParam(createParamCentered<VCVSlider>(mm2px(Vec(4.5, 66.5)), module, SERRA::ATTACK_PARAM));
        addParam(createParamCentered<VCVSlider>(mm2px(Vec(14.5, 66.5)), module, SERRA::DECAY_PARAM));
        addParam(createParamCentered<VCVSlider>(mm2px(Vec(24.5, 66.5)), module, SERRA::SUSTAIN_PARAM));
        addParam(createParamCentered<VCVSlider>(mm2px(Vec(34.5, 66.5)), module, SERRA::RELEASE_PARAM));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5, 95.25)), module, SERRA::A_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15, 95.25)), module, SERRA::D_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25, 95.25)), module, SERRA::S_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(35, 95.25)), module, SERRA::R_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(45, 95.25)), module, SERRA::SIG_INPUT));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5, 108.25)), module, SERRA::GATE_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15, 108.25)), module, SERRA::TRIG_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(25, 108.25)), module, SERRA::ENV_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(35, 108.25)), module, SERRA::EOC_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(45, 108.25)), module, SERRA::OUT_OUTPUT));

        addChild(createLightCentered<MediumLight<WhiteLight>>(mm2px(Vec(45.0, 53.75)), module, SERRA::ENV_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(45.0, 67.35)), module, SERRA::EOC_LIGHT));
        addChild(createLightCentered<MediumLight<GreenRedLight>>(mm2px(Vec(45.0, 78.75)), module, SERRA::OUT_LIGHT));
    }
};

Model *modelSERRA = createModel<SERRA, SERRAWidget>("SERRA");