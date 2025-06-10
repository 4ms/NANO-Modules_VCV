#include "plugin.hpp"
#include <componentlibrary.hpp>
#include "NANOComponents.hpp"

// Include custom envelope and shaper utilities for sound shaping.
#include "Resources/SynthTools/envelope.hpp"
#include "Resources/SynthTools/shaper.hpp"
#include "Resources/SynthTools/slewLimiter.hpp"

#define STEEPNESS 0.9f

struct ARC : Module {
    // Envelope generators, one per channel.
    ADEnvelope ENV_X, ENV_Y;
    NANO_Resources::SlewLimiter SLW_X, SLW_Y;
    float outX, outY;
    bool risingX, risingY;
    bool fallingX, fallingY;
    enum ParamIds {
        X_RISE_FREQ_PARAM,
        X_RISE_SHAPE_PARAM,
        X_FALL_FREQ_PARAM,
        X_FALL_SHAPE_PARAM,
        X_ATTV_PARAM,
        X_OFFSET_PARAM,
        X_GATE_SW,
        X_SPEED_SW,
        X_LOOP_SW,
        X_SUSTAIN_SW,
        Y_RISE_FREQ_PARAM,
        Y_RISE_SHAPE_PARAM,
        Y_FALL_FREQ_PARAM,
        Y_FALL_SHAPE_PARAM,
        Y_ATTV_PARAM,
        Y_OFFSET_PARAM,
        Y_GATE_SW,
        Y_SPEED_SW,
        Y_LOOP_SW,
        Y_SUSTAIN_SW,
        NUM_PARAMS
    };
    enum InputIds {
        X_TRIG_IN,
        X_IN,
        X_RISE_CV,
        X_EXP_CV,
        X_FALL_CV,
        X_LOOP_CV,
        X_ATTV_CV,
        Y_TRIG_IN,
        Y_IN,
        Y_RISE_CV,
        Y_EXP_CV,
        Y_FALL_CV,
        Y_LOOP_CV,
        Y_ATTV_CV,
        NUM_INPUTS
    };
    enum OutputIds {
        X_RISE_OUT,
        X_OUT,
        X_FALL_OUT,
        Y_RISE_OUT,
        Y_OUT,
        Y_FALL_OUT,
        COMP_OUT,
        SUM_OUT,
        OR_OUT,
        AND_OUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        X_RISE_LIGHT,
        X_FALL_LIGHT,
        ENUMS(X_OUT_LIGHT, 2),
        ENUMS(Y_OUT_LIGHT, 2),
        Y_RISE_LIGHT,
        Y_FALL_LIGHT,
        X_GATE_LIGHT,
        Y_GATE_LIGHT,
        NUM_LIGHTS
    };

    ARC() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        // Configure X channel parameters
        configParam(X_RISE_FREQ_PARAM, 0.f, 1.f, 0.5f, "X Rise time");
        configParam(X_FALL_FREQ_PARAM, 0.f, 1.f, 0.5f, "X Fall time");
        configParam(X_RISE_SHAPE_PARAM, 0.f, 1.f, 0.5f, "X Rise shape");
        configParam(X_FALL_SHAPE_PARAM, 0.f, 1.f, 0.5f, "X Fall shape");
        configParam(X_ATTV_PARAM, -1.f, 1.f, 0.f, "X Attenuverter");
        configParam(X_OFFSET_PARAM, -5.f, 5.f, 0.f, "X Offset");
        configParam(X_GATE_SW, 0.f, 1.f, 0.f, "X Gate");
        configParam(X_SPEED_SW, 0.f, 2.f, 1.0f, "X Frequency divider");
        configParam(X_LOOP_SW, 0.f, 1.f, 0.f, "X Loop");
        configParam(X_SUSTAIN_SW, 0.f, 1.f, 0.f, "X Sustain");

        // Configure Y channel parameters
        configParam(Y_RISE_FREQ_PARAM, 0.f, 1.f, 0.5f, "Y Rise time");
        configParam(Y_FALL_FREQ_PARAM, 0.f, 1.f, 0.5f, "Y Fall time");
        configParam(Y_RISE_SHAPE_PARAM, 0.f, 1.f, 0.5f, "Y Rise shape");
        configParam(Y_FALL_SHAPE_PARAM, 0.f, 1.f, 0.5f, "Y Fall shape");
        configParam(Y_ATTV_PARAM, -1.f, 1.f, 0.f, "Y Attenuverter");
        configParam(Y_OFFSET_PARAM, -5.f, 5.f, 0.f, "Y Offset");
        configParam(Y_GATE_SW, 0.f, 1.f, 0.f, "Y Gate");
        configParam(Y_SPEED_SW, 0.f, 2.f, 1.0f, "Y Frequency divider");
        configParam(Y_LOOP_SW, 0.f, 1.f, 0.f, "Y Loop");
        configParam(Y_SUSTAIN_SW, 0.f, 1.f, 0.f, "Y Sustain");

        // Configure X channel inputs
        configInput(X_TRIG_IN, "X Trigger");
        configInput(X_IN, "X Signal");
        configInput(X_RISE_CV, "X Rise CV");
        configInput(X_EXP_CV, "X Exponential CV");
        configInput(X_FALL_CV, "X Fall CV");
        configInput(X_LOOP_CV, "X Loop CV");
        configInput(X_ATTV_CV, "X Attenuverter CV");

        // Configure Y channel inputs
        configInput(Y_TRIG_IN, "Y Trigger");
        configInput(Y_IN, "Y Signal");
        configInput(Y_RISE_CV, "Y Rise CV");
        configInput(Y_EXP_CV, "Y Exponential CV");
        configInput(Y_FALL_CV, "Y Fall CV");
        configInput(Y_LOOP_CV, "Y Loop CV");
        configInput(Y_ATTV_CV, "Y Attenuverter CV");

        // Configure outputs
        configOutput(X_RISE_OUT, "X Rise Gate");
        configOutput(X_OUT, "X Channel");
        configOutput(X_FALL_OUT, "X Fall Gate");
        configOutput(Y_RISE_OUT, "Y Rise Gate");
        configOutput(Y_OUT, "Y Channel");
        configOutput(Y_FALL_OUT, "Y Fall Gate");
        configOutput(COMP_OUT, "Comparator");
        configOutput(SUM_OUT, "Sum");
        configOutput(OR_OUT, "OR Logic");
        configOutput(AND_OUT, "AND Logic");

        // Initialize the envelope generator for each channel.
        ENV_X.init(APP->engine->getSampleRate());
        ENV_X.setAttackShape(0.8f);
        ENV_X.setDecayShape(0.2f);
        ENV_X.setOutputLevel(10.0f);
        ENV_X.setSustainLevel(10.0f);
        ENV_X.setResetOnTrigger(false);

        ENV_Y.init(APP->engine->getSampleRate());
        ENV_Y.setAttackShape(0.8f);
        ENV_Y.setDecayShape(0.2f);
        ENV_Y.setOutputLevel(10.0f);
        ENV_Y.setSustainLevel(10.0f);
        ENV_Y.setResetOnTrigger(false);

        SLW_X.init(APP->engine->getSampleRate());
        SLW_Y.init(APP->engine->getSampleRate());
    }

    void process(const ProcessArgs &args) override {
        // X Channel Inputs
        float xTrigIn = inputs[X_TRIG_IN].getVoltage();
        float xIn = inputs[X_IN].getVoltage();
        float xRiseCV = inputs[X_RISE_CV].getVoltage();
        float xExpCV = inputs[X_EXP_CV].getVoltage();
        float xFallCV = inputs[X_FALL_CV].getVoltage();
        float xLoopCV = inputs[X_LOOP_CV].getVoltage();
        float xAttvCV = inputs[X_ATTV_CV].getVoltage();

        // Y Channel Inputs
        float yTrigIn = inputs[Y_TRIG_IN].getVoltage();
        float yIn = inputs[Y_IN].getVoltage();
        float yRiseCV = inputs[Y_RISE_CV].getVoltage();
        float yExpCV = inputs[Y_EXP_CV].getVoltage();
        float yFallCV = inputs[Y_FALL_CV].getVoltage();
        float yLoopCV = inputs[Y_LOOP_CV].getVoltage();
        float yAttvCV = inputs[Y_ATTV_CV].getVoltage();

        // X Channel Parameters
        float xRiseFreq = Shaper::shapeCurve(params[X_RISE_FREQ_PARAM].getValue() + (xRiseCV - xExpCV) / 10.f, 0.000485f, 1.65f, 0.0f, STEEPNESS);
        float xFallFreq = Shaper::shapeCurve(params[X_FALL_FREQ_PARAM].getValue() + (xFallCV - xExpCV) / 10.f, 0.000485f, 1.65f, 0.0f, STEEPNESS);
        float xRiseShape = params[X_RISE_SHAPE_PARAM].getValue();
        float xFallShape = params[X_FALL_SHAPE_PARAM].getValue();
        float xAttenuverter = params[X_ATTV_PARAM].getValue() + inputs[X_ATTV_CV].getVoltage() / 10.f;
        float xOffset = params[X_OFFSET_PARAM].getValue();
        float xSpeed = params[X_SPEED_SW].getValue();
        bool xGate = params[X_GATE_SW].getValue() > 0.5f;
        bool xLoop = params[X_LOOP_SW].getValue() + xLoopCV / 5.f > 0.5f;
        bool xSustain = params[X_SUSTAIN_SW].getValue() > 0.5f;

        // Y Channel Parameters
        float yRiseFreq = Shaper::shapeCurve(params[Y_RISE_FREQ_PARAM].getValue() + (yRiseCV - yExpCV) / 10.f, 0.000485f, 1.65f, 0.0f, STEEPNESS);
        float yFallFreq = Shaper::shapeCurve(params[Y_FALL_FREQ_PARAM].getValue() + (yFallCV - yExpCV) / 10.f, 0.000485f, 1.65f, 0.0f, STEEPNESS);
        float yRiseShape = params[Y_RISE_SHAPE_PARAM].getValue();
        float yFallShape = params[Y_FALL_SHAPE_PARAM].getValue();
        float yAttenuverter = params[Y_ATTV_PARAM].getValue() + inputs[Y_ATTV_CV].getVoltage() / 10.f;
        float yOffset = params[Y_OFFSET_PARAM].getValue();
        float ySpeed = params[Y_SPEED_SW].getValue();
        bool yGate = params[Y_GATE_SW].getValue() > 0.5f;
        bool yLoop = params[Y_LOOP_SW].getValue() + yLoopCV / 5.f > 0.5f;
        bool ySustain = params[Y_SUSTAIN_SW].getValue() > 0.5f;

        // Adjust rise and fall times based on the speed switch position for the X channel
        if (xSpeed == 0) {
            xRiseFreq *= 200.0;
            xFallFreq *= 200.0;
        } else if (xSpeed == 1) {
            // No adjustment needed for the center position
        } else if (xSpeed == 2) {
            xRiseFreq *= 20.0;
            xFallFreq *= 20.0;
        }

        // Adjust rise and fall times based on the speed switch position for the Y channel
        if (ySpeed == 0) {
            yRiseFreq *= 200.0;
            yFallFreq *= 200.0;
        } else if (ySpeed == 1) {
            // No adjustment needed for the center position
        } else if (ySpeed == 2) {
            yRiseFreq *= 20.0;
            yFallFreq *= 20.0;
        }

        // Set the adjusted rise and fall times to the envelope & slew generator.
        ENV_X.setAttack(xRiseFreq);
        ENV_X.setDecay(xFallFreq);
        SLW_X.setRise(xRiseFreq);
        SLW_X.setFall(xFallFreq);
        ENV_Y.setAttack(yRiseFreq);
        ENV_Y.setDecay(yFallFreq);
        SLW_Y.setRise(yRiseFreq);
        SLW_Y.setFall(yFallFreq);

        // Set the adjusted rise and fall shapes to the envelope & slew generator.
        ENV_X.setAttackShape(xRiseShape);
        ENV_X.setDecayShape(xFallShape);
        SLW_X.setRiseShape(xRiseShape);
        SLW_X.setFallShape(xFallShape);
        ENV_Y.setAttackShape(yRiseShape);
        ENV_Y.setDecayShape(yFallShape);
        SLW_Y.setRiseShape(yRiseShape);
        SLW_Y.setFallShape(yFallShape);

        // Enable or disable looping based on the loop switch position.
        ENV_X.setLooping(params[X_LOOP_SW].getValue() + inputs[X_LOOP_CV].getVoltage());
        ENV_Y.setLooping(params[Y_LOOP_SW].getValue() + inputs[Y_LOOP_CV].getVoltage());

        // Enable or disable sustain based on the loop switch position.
        ENV_X.setSustainActive(params[X_SUSTAIN_SW].getValue());
        ENV_Y.setSustainActive(params[Y_SUSTAIN_SW].getValue());

        if (inputs[X_IN].isConnected()) {
            // Process the slew limiter for the external input signal
            outX = (SLW_X.process(xIn) + xOffset) * xAttenuverter;
            // Set the rising variable from the Slew Limiter
            risingX = SLW_X.getRising();
            fallingX = SLW_X.getFalling();
        } else {
            // Process the envelope generator with the current trigger voltage.
            ENV_X.process(inputs[X_TRIG_IN].getVoltage() + xGate);
            // Apply attenuation and inversion (if necessary) to the envelope's current value
            outX = (ENV_X.getCurveOutput() + xOffset) * xAttenuverter;  // Attenuation applied to X output
            // Set the rising variable from the Envelope
            risingX = ENV_X.isRising();
            fallingX = ENV_X.isFalling();
        }

        if (inputs[Y_IN].isConnected()) {
            // Process the slew limiter for the external input signal
            outY = (SLW_Y.process(yIn) + yOffset) * yAttenuverter;
            // Set the rising variable from the Slew Limiter
            risingY = SLW_Y.getRising();
            fallingY = SLW_Y.getFalling();
        } else {
            // Process the envelope generator with the current trigger voltage.
            ENV_Y.process(inputs[Y_TRIG_IN].getVoltage() + yGate);
            // Apply attenuation and inversion (if necessary) to the envelope's current value
            outY = (ENV_Y.getCurveOutput() + yOffset) * yAttenuverter;  // Attenuation applied to Y output
            // Set the rising variable from the Envelope
            risingY = ENV_Y.isRising();
            fallingY = ENV_Y.isFalling();
        }

        // Output the envelope's current value with attenuation and inversion applied
        outputs[X_OUT].setVoltage(outX);
        outputs[Y_OUT].setVoltage(outY);

        // Output the envelope's rising & falling value
        outputs[X_RISE_OUT].setVoltage(risingX * 10.0f);
        outputs[X_FALL_OUT].setVoltage(fallingX * 10.0f);

        outputs[Y_RISE_OUT].setVoltage(risingY * 10.0f);
        outputs[Y_FALL_OUT].setVoltage(fallingY * 10.0f);

        // Logic Section Outputs
        // Comparator (X > Y)
        if (outX > outY) {
            outputs[COMP_OUT].setVoltage(10.f);  // High voltage if X > Y
        } else {
            outputs[COMP_OUT].setVoltage(0.f);  // Low voltage if X <= Y
        }

        // Sum (X + Y)
        outputs[SUM_OUT].setVoltage((outX + outY) / 2.0f);

        // OR logic (maximum of X or Y)
        float orOutput = std::max(outX, outY);
        outputs[OR_OUT].setVoltage(orOutput);

        // AND logic (minimum of X and Y)
        float andOutput = std::min(outX, outY);
        outputs[AND_OUT].setVoltage(andOutput);

        // Set the brightness of the corresponding lights.
        lights[X_RISE_LIGHT].setBrightness(risingX ? 1.0f : 0.0f);
        lights[X_GATE_LIGHT].setBrightness(risingX ? 1.0f : 0.0f);
        lights[X_FALL_LIGHT].setBrightness(fallingX ? 1.0f : 0.0f);
        lights[X_OUT_LIGHT + 0].setSmoothBrightness(fmaxf(0.0, outX / 5.0), 0.01f);
        lights[X_OUT_LIGHT + 1].setSmoothBrightness(fmaxf(0.0, -outX / 5.0), 0.01f);

        lights[Y_RISE_LIGHT].setBrightness(risingY ? 1.0f : 0.0f);
        lights[Y_GATE_LIGHT].setBrightness(risingY ? 1.0f : 0.0f);
        lights[Y_FALL_LIGHT].setBrightness(fallingY ? 1.0f : 0.0f);
        lights[Y_OUT_LIGHT + 0].setSmoothBrightness(fmaxf(0.0, outY / 5.0), 0.01f);
        lights[Y_OUT_LIGHT + 1].setSmoothBrightness(fmaxf(0.0, -outY / 5.0), 0.01f);
    }
};

struct ARCWidget : ModuleWidget {
    ARCWidget(ARC *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ARC.svg")));

        addChild(createWidget<ScrewSilver>(Vec(12.86, 1.5)));
        addChild(createWidget<ScrewSilver>(Vec(12.86, 363.5)));
        addChild(createWidget<ScrewSilver>(Vec(331.86, 1.5)));
        addChild(createWidget<ScrewSilver>(Vec(331.86, 363.5)));

        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(12.36, 18.75)), module, ARC::X_RISE_SHAPE_PARAM));
        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(12.36, 40.75)), module, ARC::X_FALL_SHAPE_PARAM));
        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(109.36, 18.75)), module, ARC::Y_RISE_SHAPE_PARAM));
        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(109.36, 40.75)), module, ARC::Y_FALL_SHAPE_PARAM));

        addParam(createParamCentered<Davies1900hBlackKnob>(mm2px(Vec(35.36, 22.75)), module, ARC::X_RISE_FREQ_PARAM));
        addParam(createParamCentered<Davies1900hBlackKnob>(mm2px(Vec(35.36, 44.75)), module, ARC::X_FALL_FREQ_PARAM));
        addParam(createParamCentered<Davies1900hBlackKnob>(mm2px(Vec(86.36, 22.75)), module, ARC::Y_RISE_FREQ_PARAM));
        addParam(createParamCentered<Davies1900hBlackKnob>(mm2px(Vec(86.36, 44.75)), module, ARC::Y_FALL_FREQ_PARAM));

        addParam(createParamCentered<Davies1900hLargeBlackKnob>(mm2px(Vec(33.86, 69.75)), module, ARC::X_ATTV_PARAM));
        addParam(createParamCentered<Davies1900hLargeBlackKnob>(mm2px(Vec(87.86, 69.75)), module, ARC::Y_ATTV_PARAM));

        addParam(createParamCentered<Trimpot>(mm2px(Vec(11.36, 76)), module, ARC::X_OFFSET_PARAM));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(110.36, 76)), module, ARC::Y_OFFSET_PARAM));

        addParam(createParamCentered<PB61303>(mm2px(Vec(11.36, 61.75)), module, ARC::X_GATE_SW));
        addChild(createLightCentered<PB61303Light<WhiteLight>>(mm2px(Vec(11.36, 61.75)), module, ARC::X_GATE_LIGHT));

        addParam(createParamCentered<PB61303>(mm2px(Vec(110.36, 61.75)), module, ARC::Y_GATE_SW));
        addChild(createLightCentered<PB61303Light<WhiteLight>>(mm2px(Vec(110.36, 61.75)), module, ARC::Y_GATE_LIGHT));

        addParam(createParamCentered<NANOComponents::BarkSwitchSmall2P>(mm2px(Vec(55.36, 44.75)), module, ARC::X_LOOP_SW));
        addParam(createParamCentered<NANOComponents::BarkSwitchSmall2P>(mm2px(Vec(66.36, 44.75)), module, ARC::Y_LOOP_SW));

        addParam(createParamCentered<NANOComponents::BarkSwitchSmall2P>(mm2px(Vec(55.36, 58.25)), module, ARC::X_SUSTAIN_SW));
        addParam(createParamCentered<NANOComponents::BarkSwitchSmall2P>(mm2px(Vec(66.36, 58.25)), module, ARC::Y_SUSTAIN_SW));

        addParam(createParamCentered<NANOComponents::BarkSwitchSmallVert3P>(mm2px(Vec(55.36, 32.25)), module, ARC::X_SPEED_SW));
        addParam(createParamCentered<NANOComponents::BarkSwitchSmallVert3P>(mm2px(Vec(66.36, 32.25)), module, ARC::Y_SPEED_SW));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.86, 95.25)), module, ARC::X_TRIG_IN));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(19.86, 95.25)), module, ARC::X_RISE_CV));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.86, 95.25)), module, ARC::X_EXP_CV));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(41.86, 95.25)), module, ARC::X_FALL_CV));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.86, 108.25)), module, ARC::X_IN));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(19.86, 108.25)), module, ARC::X_RISE_OUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30.86, 108.25)), module, ARC::X_OUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(41.86, 108.25)), module, ARC::X_FALL_OUT));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(112.86, 95.25)), module, ARC::Y_TRIG_IN));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(79.86, 95.25)), module, ARC::Y_RISE_CV));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(90.86, 95.25)), module, ARC::Y_EXP_CV));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(101.86, 95.25)), module, ARC::Y_FALL_CV));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(112.86, 108.25)), module, ARC::Y_IN));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(79.86, 108.25)), module, ARC::Y_RISE_OUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(90.86, 108.25)), module, ARC::Y_OUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(101.86, 108.25)), module, ARC::Y_FALL_OUT));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(55.36, 69.25)), module, ARC::X_LOOP_CV));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(66.36, 69.25)), module, ARC::Y_LOOP_CV));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(55.36, 82.25)), module, ARC::X_ATTV_CV));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(66.36, 82.25)), module, ARC::Y_ATTV_CV));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(55.36, 95.25)), module, ARC::COMP_OUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(66.36, 95.25)), module, ARC::SUM_OUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(55.36, 108.25)), module, ARC::OR_OUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(66.36, 108.25)), module, ARC::AND_OUT));

        addChild(createLightCentered<MediumLight<WhiteLight>>(mm2px(Vec(55.36, 10.5)), module, ARC::X_RISE_LIGHT));
        addChild(createLightCentered<MediumLight<GreenRedLight>>(mm2px(Vec(55.36, 16.5)), module, ARC::X_OUT_LIGHT));
        addChild(createLightCentered<MediumLight<WhiteLight>>(mm2px(Vec(55.36, 23)), module, ARC::X_FALL_LIGHT));

        addChild(createLightCentered<MediumLight<WhiteLight>>(mm2px(Vec(66.36, 10.5)), module, ARC::Y_RISE_LIGHT));
        addChild(createLightCentered<MediumLight<GreenRedLight>>(mm2px(Vec(66.36, 16.5)), module, ARC::Y_OUT_LIGHT));
        addChild(createLightCentered<MediumLight<WhiteLight>>(mm2px(Vec(66.36, 23)), module, ARC::Y_FALL_LIGHT));
    }
};

Model *modelARC = createModel<ARC, ARCWidget>("ARC");