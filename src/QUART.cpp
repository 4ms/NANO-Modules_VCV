// Include necessary headers for the plugin, standard components, and custom NANOComponents.
#include "plugin.hpp"
#include <componentlibrary.hpp>
#include "NANOComponents.hpp"

// Include custom envelope and shaper utilities for sound shaping.
#include "Resources/SynthTools/envelope.hpp"
#include "Resources/SynthTools/shaper.hpp"

// Define constants for the number of channels this module will handle and a fixed steepness value for shaping curves.
#define CHANNELS  4
#define STEEPNESS 0.45f

// Define the QUART module, inheriting from Module, the base class for all modules in VCVRack.
struct QUART : Module
{
    // Array of envelope generators, one per channel.
    ADEnvelope ENV[CHANNELS];

    // Arrays for storing frequency adjustment values, one per channel for rise and fall.
    float riseFreq[CHANNELS];
    float fallFreq[CHANNELS];

    // Array for frequency multiplier settings, one per channel.
    int freqMult[CHANNELS];

    // Enumerations to define IDs for the module's parameters, inputs, outputs, and lights.
    enum ParamIds
    {
        ENUMS(RISE, 4), // Rise time parameters for each channel.
        ENUMS(FALL, 4), // Fall time parameters for each channel.
        ENUMS(SW, 4),   // Switch positions for frequency division, one per channel.
        NUM_PARAMS     // Total number of parameters.
    };
    enum InputIds
    {
        ENUMS(TRIG, 4), // Trigger inputs for each channel.
        NUM_INPUTS     // Total number of inputs.
    };
    enum OutputIds
    {
        ENUMS(OUT, 4), // Output for each channel.
        NUM_OUTPUTS    // Total number of outputs.
    };
    enum LightIds
    {
        ENUMS(LIGHT, 4), // Light indicators for each channel.
        NUM_LIGHTS       // Total number of lights.
    };

    // Constructor for the QUART module, initializing parameters and envelope generators.
    QUART()
    {
        // Basic module configuration.
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        for (int i = 0; i < CHANNELS; i++){
            // Configure parameters for rise time, fall time, and switch position with default values.
            configParam(RISE + i, 0.f, 1.f, 0.5f, "Rise time");
            configParam(FALL + i, 0.f, 1.f, 0.5f, "Fall time");
            configParam(SW + i, 0.f, 2.f, 1.0f, "Frequency divider");
            configInput(TRIG + i, "Trigger " + std::to_string(i + 1));
            configOutput(OUT + i, "Channel " + std::to_string(i + 1));

            // Initialize the envelope generator for each channel.
            ENV[i].init(APP->engine->getSampleRate());
            ENV[i].setAttackShape(0.8f);
            ENV[i].setDecayShape(0.2f);
            ENV[i].setOutputLevel(8.0f);
            ENV[i].setResetOnTrigger(false);
        }  
    }

    // Override the process method to implement the module's functionality.
    void process(const ProcessArgs &args) override
    {
        // TO DO: Match QUART rise & fall times with the real one, also curve skew
        for (int i = 0; i < CHANNELS; i++){
            // Calculate rise and fall times based on the parameter settings and the shaping curve.
            float riseTime = Shaper::shapeCurve(params[RISE + i].getValue(), 0.0001087f, 0.65f, 0.0f, STEEPNESS);
            float fallTime = Shaper::shapeCurve(params[FALL + i].getValue(), 0.0001087f, 0.65f, 0.0f, STEEPNESS);

            // Read the switch position to adjust rise and fall times based on frequency division settings.
            float switchPosition = params[SW + i].getValue();
            if (switchPosition == 0) {
                riseTime *= 100.0;
                fallTime *= 100.0;
            } else if (switchPosition == 1) {
                // No adjustment needed for the center position.
            } else if (switchPosition == 2) {
                riseTime *= 10.0;
                fallTime *= 10.0;
            }

            // Set the adjusted rise and fall times to the envelope generator.
            ENV[i].setAttack(riseTime);
            ENV[i].setDecay(fallTime);

            // Enable or disable looping based on whether a trigger input is connected.
            ENV[i].setLooping(!inputs[TRIG + i].isConnected());

            // Process the envelope generator with the current trigger voltage.
            ENV[i].process(inputs[TRIG + i].getVoltage());

            // Output the envelope's current value and set the brightness of the corresponding light.
            outputs[OUT + i].setVoltage(ENV[i].getCurveOutput());
            lights[LIGHT + i].setBrightnessSmooth(ENV[i].getCurveOutput() / 8.0f, 0.01f);
        }
    }
};


struct QUARTWidget : ModuleWidget
{
    QUARTWidget(QUART *module)
    {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/QUART.svg")));

        addChild(createWidget<ScrewSilver>(Vec(14, 1.5)));
        //addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        //addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(120, 363.5)));

        for (int i = 0; i < 4; i++){
            addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(10, 15.30 + (i * 21.0))), module, QUART::RISE + i));
            addParam(createParamCentered<Davies1900hBlackKnob>(mm2px(Vec(40, 15.30 + (i * 21.0))), module, QUART::FALL + i));
            addParam(createParamCentered<NANOComponents::BarkSwitchSmallSide3P>(mm2px(Vec(25, 15.25 + (i * 21.0))), module, QUART::SW + i));
            addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7 + i * 12, 95.25)), module, QUART::TRIG + i));
            addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7 + i * 12, 108.25)), module, QUART::OUT + i));
            addChild(createLightCentered<MediumLight<WhiteLight>>(mm2px(Vec(25, 9.25 + (i * 21.0))), module, QUART::LIGHT + i));
        }
    }
};

Model *modelQUART = createModel<QUART, QUARTWidget>("QUART");