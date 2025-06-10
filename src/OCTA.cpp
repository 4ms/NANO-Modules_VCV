#include "plugin.hpp"
#include "NANOComponents.hpp"

// Include custom filter class
#include "Resources/DaisySP/Source/Drums/analogbassdrum.h"
#include "Resources/DaisySP/Source/Drums/analogsnaredrum.h"
#include "Resources/DaisySP/Source/Drums/hihat.h"
#include "Resources/DaisySP/Source/Dynamics/compressor.h"
#include "Resources/DaisySP/Source/Utility/dcblock.h"
#include "Resources/SynthTools/filter.hpp"
#include "Resources/SynthTools/shaper.hpp"
#include "Resources/SynthTools/slewLimiter.hpp"

#define LED_SMOOTHING 0.00001f
#define MAX_GAIN 4.0f
#define SAMPLE_RATE 44100.0f

using namespace daisysp;

struct OCTA : Module {
    dsp::SchmittTrigger trigKick, trigSnare, trigHhc, trigHho;

    AnalogBassDrum kick;
    AnalogSnareDrum snare;
    HiHat<SquareNoise, LinearVCA, true> hhc;
    HiHat<SquareNoise, LinearVCA, true> hho;
    Compressor comp;
    Filter tilt;
    DcBlock dc;
    NANO_Resources::SlewLimiter slew;

    float volKick = 0.0f;
    float volSnare = 0.0f;
    float volHhc = 0.0f;
    float volHho = 0.0f;

    enum ParamIds {
        DRV_KICK,
        DRV_SNARE,
        DRV_HHC,
        DRV_HHO,
        MUTE_KICK,
        MUTE_SNARE,
        MUTE_HHC,
        MUTE_HHO,
        TONE_KICK,
        TUNE_KICK,
        DECAY_KICK,
        TONE_SNARE,
        SNAP_SNARE,
        NOISE_SNARE,
        TONE_HH,
        DECAY_HH,
        COMP_MASTER,
        TILT_MASTER,
        NUM_PARAMS
    };
    enum InputIds {
        TRIG_KICK,
        TRIG_SNARE,
        TRIG_HHC,
        TRIG_HHO,
        CV_KICK,
        CV_SNARE,
        CV_HHC,
        CV_HHO,
        NUM_INPUTS
    };
    enum OutputIds {
        OUT_KICK,
        OUT_SNARE,
        OUT_HHC,
        OUT_HHO,
        OUT_SIDE,
        OUT_MIX,
        NUM_OUTPUTS
    };
    enum LightIds {
        KICK_TRIG_LIGHT,
        SNARE_TRIG_LIGHT,    // canal 0 (rojo)
        SNARE_TRIG_LIGHT_G,  // canal 1 (verde)
        HHC_TRIG_LIGHT,
        HHO_TRIG_LIGHT,
        DRV_KICK_LIGHT,
        DRV_SNARE_LIGHT,    // canal 0 (rojo)
        DRV_SNARE_LIGHT_G,  // canal 1 (verde)
        DRV_HHC_LIGHT,
        DRV_HHO_LIGHT,
        NUM_LIGHTS
    };

    OCTA() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        // === Param config ===
        configParam(DRV_KICK, 0.f, 1.f, 0.5f, "Kick Drive");
        configParam(DRV_SNARE, 0.f, 1.f, 0.5f, "Snare Drive");
        configParam(DRV_HHC, 0.f, 1.f, 0.5f, "Closed Hat Drive");
        configParam(DRV_HHO, 0.f, 1.f, 0.5f, "Open Hat Drive");

        configParam(MUTE_KICK, 0.f, 1.f, 1.f, "Kick Mute");
        configParam(MUTE_SNARE, 0.f, 1.f, 1.f, "Snare Mute");
        configParam(MUTE_HHC, 0.f, 1.f, 1.f, "Closed Hat Mute");
        configParam(MUTE_HHO, 0.f, 1.f, 1.f, "Open Hat Mute");

        configParam(TONE_KICK, 0.f, 1.f, 1.0f, "Kick Tone");
        configParam(TUNE_KICK, 0.f, 1.f, 0.5f, "Kick Tune");
        configParam(DECAY_KICK, 0.f, 1.f, 0.8f, "Kick Decay");

        configParam(TONE_SNARE, 0.f, 1.f, 0.5f, "Snare Tone");
        configParam(SNAP_SNARE, 0.f, 1.f, 1.0f, "Snare Snap");
        configParam(NOISE_SNARE, 0.f, 1.f, 1.0f, "Snare Noise");

        configParam(TONE_HH, 0.f, 1.f, 0.5f, "Hat Tone");
        configParam(DECAY_HH, 0.f, 1.f, 0.8f, "Hat Decay");

        configParam(COMP_MASTER, 0.f, 1.f, 0.5f, "Master Compressor");
        configParam(TILT_MASTER, 0.f, 1.f, 0.5f, "Master Tilt");

        // === Input names ===
        configInput(TRIG_KICK, "Kick Trigger");
        configInput(TRIG_SNARE, "Snare Trigger");
        configInput(TRIG_HHC, "Closed Hat Trigger");
        configInput(TRIG_HHO, "Open Hat Trigger");

        configInput(CV_KICK, "Kick Volume CV");
        configInput(CV_SNARE, "Snare Volume CV");
        configInput(CV_HHC, "Closed Hat Volume CV");
        configInput(CV_HHO, "Open Hat Volume CV");

        // === Output names ===
        configOutput(OUT_KICK, "Kick Out");
        configOutput(OUT_SNARE, "Snare Out");
        configOutput(OUT_HHC, "Closed Hat Out");
        configOutput(OUT_HHO, "Open Hat Out");
        configOutput(OUT_SIDE, "Sidechain Out");
        configOutput(OUT_MIX, "Mix Out");

        kick.Init(SAMPLE_RATE);
        kick.SetFreq(40.0f);
        kick.SetDecay(0.5f);
        kick.SetTone(0.5f);
        kick.SetSustain(false);
        kick.SetAttackFmAmount(0.0f);
        kick.SetSelfFmAmount(0.0f);
        snare.Init(SAMPLE_RATE);
        hhc.Init(SAMPLE_RATE);
        hhc.SetNoisiness(0.82f);
        hho.Init(SAMPLE_RATE);
        hho.SetNoisiness(0.82f);
        tilt.init();
        comp.Init(SAMPLE_RATE);
        slew.init(SAMPLE_RATE);
        slew.setRiseShape(0.01f);
        slew.setFallShape(0.01f);
        slew.setRise(0.001f);
        slew.setFall(0.1f);
        dc.Init(SAMPLE_RATE);
    }

    void process(const ProcessArgs &args) override {
        // === Read knob values ===
        if (inputs[CV_KICK].isConnected()) {
            volKick = (inputs[CV_KICK].getVoltage() / 3.0f) * params[DRV_KICK].getValue();
        } else {
            volKick = Shaper::mapToRange(params[DRV_KICK].getValue(), 0.0f, 1.0f, 0.0f, 0.75f, MAX_GAIN);
        }
        if (inputs[CV_SNARE].isConnected()) {
            volSnare = (inputs[CV_SNARE].getVoltage() / 3.0f) * params[DRV_SNARE].getValue();
        } else {
            volSnare = Shaper::mapToRange(params[DRV_SNARE].getValue(), 0.0f, 1.0f, 0.0f, 0.75f, MAX_GAIN);
        }
        if (inputs[CV_HHC].isConnected()) {
            volHhc = (inputs[CV_HHC].getVoltage() / 3.0f) * params[DRV_HHC].getValue();
        } else {
            volHhc = Shaper::mapToRange(params[DRV_HHC].getValue(), 0.0f, 1.0f, 0.0f, 0.75f, MAX_GAIN);
        }
        if (inputs[CV_HHO].isConnected()) {
            volHho = (inputs[CV_HHO].getVoltage() / 3.0f) * params[DRV_HHO].getValue();
        } else {
            volHho = Shaper::mapToRange(params[DRV_HHO].getValue(), 0.0f, 1.0f, 0.0f, 0.75f, MAX_GAIN);
        }
        float toneKick = params[TONE_KICK].getValue();
        float tuneKick = params[TUNE_KICK].getValue();
        float decayKick = params[DECAY_KICK].getValue();

        float toneSnare = params[TONE_SNARE].getValue();
        float snapSnare = params[SNAP_SNARE].getValue();
        float noiseSnare = params[NOISE_SNARE].getValue();

        float toneHh = 0.8f + params[TONE_HH].getValue() / 5.0f;
        float decayHh = params[DECAY_HH].getValue();

        float compMaster = params[COMP_MASTER].getValue();
        float tiltMaster = params[TILT_MASTER].getValue() * 2.0f - 1.0f;

        // === Read mute switches ===
        bool muteKick = params[MUTE_KICK].getValue() < 0.5f;
        bool muteSnare = params[MUTE_SNARE].getValue() < 0.5f;
        bool muteHhc = params[MUTE_HHC].getValue() < 0.5f;
        bool muteHho = params[MUTE_HHO].getValue() < 0.5f;

        // === Update drum parameters ===
        kick.SetTone((toneKick * toneKick) * 0.35f);
        kick.SetDecay(decayKick * 1.5f);
        kick.SetFreq(35.0f + tuneKick * 50.0f);
        kick.SetAccent(0.7f);

        snare.SetFreq(250.0f + toneSnare * 75.0f);
        snare.SetDecay(noiseSnare / 2.5f);
        snare.SetTone(0.0f);
        snare.SetSnappy(snapSnare / 3.0f);
        snare.SetAccent(0.7f);

        hhc.SetAccent(0.7f);
        hho.SetAccent(0.7f);
        hhc.SetTone(toneHh);
        hho.SetTone(toneHh);
        hhc.SetFreq(3250.0f);
        hho.SetFreq(3250.0f);
        hhc.SetDecay(0.75f);
        hho.SetDecay(0.75f + decayHh * 0.35f);
        hhc.SetNoisiness(0.9f);
        hho.SetNoisiness(0.9f);

        // Set the compressor variables depending on the parameter setting
        float compThres = compMaster * -10.0f;
        float compRatio = compMaster * 4.0f + 1.0f;
        float compMakeup = compMaster * 22.0f;

        // Write variables to the compressor classes
        comp.SetAttack(0.01f);
        comp.SetRelease(0.01f);
        comp.SetRatio(compRatio);
        comp.SetThreshold(compThres);
        comp.SetMakeup(compMakeup);

        // --- 1. read all trigger inputs once ---
        bool kickTrig = trigKick.process(inputs[TRIG_KICK].getVoltage());
        bool snareTrig = trigSnare.process(inputs[TRIG_SNARE].getVoltage());
        bool hhcTrig = trigHhc.process(inputs[TRIG_HHC].getVoltage());
        bool hhoTrig = trigHho.process(inputs[TRIG_HHO].getVoltage());

        // === Triggers and lights ===
        if (kickTrig) {
            kick.Trig();
            lights[KICK_TRIG_LIGHT].setSmoothBrightness(1.0f, LED_SMOOTHING);
        } else {
            lights[KICK_TRIG_LIGHT].setSmoothBrightness(0.0f, LED_SMOOTHING);
        }

        if (snareTrig) {
            snare.Trig();
            lights[SNARE_TRIG_LIGHT].setSmoothBrightness(1.0f, LED_SMOOTHING);
            lights[SNARE_TRIG_LIGHT_G].setSmoothBrightness(1.0f, LED_SMOOTHING);

        } else {
            lights[SNARE_TRIG_LIGHT].setSmoothBrightness(0.0f, LED_SMOOTHING);
            lights[SNARE_TRIG_LIGHT_G].setSmoothBrightness(0.0f, LED_SMOOTHING);
        }

        if (hhcTrig) {
            hhc.Trig();
            lights[HHC_TRIG_LIGHT].setSmoothBrightness(1.0f, LED_SMOOTHING);
        } else {
            lights[HHC_TRIG_LIGHT].setSmoothBrightness(0.0f, LED_SMOOTHING);
        }

        if (hhoTrig && !hhcTrig) {
            hho.Trig();
            lights[HHO_TRIG_LIGHT].setSmoothBrightness(1.0f, LED_SMOOTHING);
        } else {
            lights[HHO_TRIG_LIGHT].setSmoothBrightness(0.0f, LED_SMOOTHING);
        }

        // === Process voices ===
        float kickOut = muteKick ? 0.f : kick.Process() * 45.0f;
        float snareOut = muteSnare ? 0.f : snare.Process() * 4.0f;
        float hhcOut = muteHhc ? 0.f : hhc.Process() * 1.0f;
        float hhoOut = muteHho ? 0.f : hho.Process() * 1.0f;

        float drvKick = kickOut * clamp(volKick, 0.0f, MAX_GAIN);
        float drvSnare = snareOut * clamp(volSnare, 0.0f, MAX_GAIN);
        float drvHhc = hhcOut * clamp(volHhc, 0.0f, MAX_GAIN);
        float drvHho = hhoOut * clamp(volHho, 0.0f, MAX_GAIN);

        float clpKick = Shaper::softClip(drvKick, 4.0f);
        float clpSnare = dc.Process(Shaper::softClip(drvSnare, 4.0f));
        float clpHhc = Shaper::softClip(drvHhc, 4.0f);
        float clpHho = Shaper::softClip(drvHho, 4.0f);

        if (drvKick > 8.0f) {
            lights[DRV_KICK_LIGHT].setSmoothBrightness(1.0f, LED_SMOOTHING);
        } else {
            lights[DRV_KICK_LIGHT].setSmoothBrightness(0.0f, LED_SMOOTHING);
        }

        if (drvSnare > 5.0f) {
            lights[DRV_SNARE_LIGHT].setSmoothBrightness(1.0f, LED_SMOOTHING);
            lights[DRV_SNARE_LIGHT_G].setSmoothBrightness(1.0f, LED_SMOOTHING);
        } else {
            lights[DRV_SNARE_LIGHT].setSmoothBrightness(0.0f, LED_SMOOTHING);
            lights[DRV_SNARE_LIGHT_G].setSmoothBrightness(0.0f, LED_SMOOTHING);
        }

        if (drvHhc > 5.0f) {
            lights[DRV_HHC_LIGHT].setSmoothBrightness(1.0f, LED_SMOOTHING);
        } else {
            lights[DRV_HHC_LIGHT].setSmoothBrightness(0.0f, LED_SMOOTHING);
        }

        if (drvHho > 5.0f) {
            lights[DRV_HHO_LIGHT].setSmoothBrightness(1.0f, LED_SMOOTHING);
        } else {
            lights[DRV_HHO_LIGHT].setSmoothBrightness(0.0f, LED_SMOOTHING);
        }

        // === Individual outputs ===
        outputs[OUT_KICK].setVoltage(clpKick);
        outputs[OUT_SNARE].setVoltage(clpSnare);
        outputs[OUT_HHC].setVoltage(clpHhc);
        outputs[OUT_HHO].setVoltage(clpHho);

        float mix = clpKick + clpSnare + clpHhc + clpHho;

        // === Mix output ===
        tilt.setInput(mix);

        // 1) F c mapeada 400 Hz – 12 kHz
        float fc = clamp(400.0f + 9500.0f * fabsf(tiltMaster), 200.0f, 12000.0f);
        tilt.setCutoff(fc / SAMPLE_RATE);  // 0-1
        tilt.setResonance(0.0f);
        tilt.process();

        // 2) Cross-fade normalizado entre LPF y HPF
        float low = tilt.getLowPass();
        float high = tilt.getHighPass();

        float lpMix = 0.5f * (1.f - tiltMaster);  // -1 →1,  +1 →0
        float hpMix = 1.f - lpMix;

        float tiltOut = low * lpMix + high * hpMix;

        // 3) Compresor + soft-clip
        float mixOut = Shaper::softClip(comp.Process(tiltOut), 6.0f);
        outputs[OUT_MIX].setVoltage(mixOut);

        float env = slew.process(mixOut);
        outputs[OUT_SIDE].setVoltage(env);
    }
};

struct OCTAWidget : ModuleWidget {
    OCTAWidget(OCTA *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/OCTA.svg")));

        addChild(createWidget<ScrewSilver>(Vec(15.44, 1.5)));
        addChild(createWidget<ScrewSilver>(Vec(15.44, 363)));
        addChild(createWidget<ScrewSilver>(Vec(239.94, 1.5)));
        addChild(createWidget<ScrewSilver>(Vec(239.94, 363)));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.25, 14.50)), module, OCTA::TRIG_KICK));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(29.25, 14.50)), module, OCTA::TRIG_SNARE));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(52.25, 14.50)), module, OCTA::TRIG_HHC));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(75.25, 14.50)), module, OCTA::TRIG_HHO));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.25, 27.50)), module, OCTA::CV_KICK));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(29.25, 27.50)), module, OCTA::CV_SNARE));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(52.25, 27.50)), module, OCTA::CV_HHC));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(75.25, 27.50)), module, OCTA::CV_HHO));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(16.25, 14.50)), module, OCTA::OUT_KICK));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(39.25, 14.50)), module, OCTA::OUT_SNARE));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(62.25, 14.50)), module, OCTA::OUT_HHC));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(85.25, 14.50)), module, OCTA::OUT_HHO));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(75.25, 108.25)), module, OCTA::OUT_SIDE));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(85.25, 108.25)), module, OCTA::OUT_MIX));

        addParam(createParamCentered<NANOComponents::BarkSwitchSmall2P>(mm2px(Vec(16.25, 27.50)), module, OCTA::MUTE_KICK));
        addParam(createParamCentered<NANOComponents::BarkSwitchSmall2P>(mm2px(Vec(39.25, 27.50)), module, OCTA::MUTE_SNARE));
        addParam(createParamCentered<NANOComponents::BarkSwitchSmall2P>(mm2px(Vec(62.25, 27.50)), module, OCTA::MUTE_HHC));
        addParam(createParamCentered<NANOComponents::BarkSwitchSmall2P>(mm2px(Vec(85.25, 27.50)), module, OCTA::MUTE_HHO));

        addParam(createParamCentered<Davies1900hBlackKnob>(mm2px(Vec(11.35, 46.0)), module, OCTA::DRV_KICK));
        addParam(createParamCentered<Davies1900hBlackKnob>(mm2px(Vec(34.35, 46.0)), module, OCTA::DRV_SNARE));
        addParam(createParamCentered<Davies1900hBlackKnob>(mm2px(Vec(57.35, 46.0)), module, OCTA::DRV_HHC));
        addParam(createParamCentered<Davies1900hBlackKnob>(mm2px(Vec(80.35, 46.0)), module, OCTA::DRV_HHO));

        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(11.35, 66.0)), module, OCTA::TONE_KICK));
        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(11.35, 86.0)), module, OCTA::TUNE_KICK));
        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(11.35, 106.0)), module, OCTA::DECAY_KICK));

        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(34.35, 66.0)), module, OCTA::TONE_SNARE));
        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(34.35, 86.0)), module, OCTA::SNAP_SNARE));
        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(34.35, 106.0)), module, OCTA::NOISE_SNARE));

        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(57.35, 66.0)), module, OCTA::TONE_HH));
        addParam(createParamCentered<Davies1900hBlackKnob>(mm2px(Vec(57.35, 86.0)), module, OCTA::COMP_MASTER));
        addParam(createParamCentered<Davies1900hBlackKnob>(mm2px(Vec(57.35, 106.0)), module, OCTA::TILT_MASTER));
        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(80.35, 66.0)), module, OCTA::DECAY_HH));

        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(73, 95.5)), module, OCTA::KICK_TRIG_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(77.75, 95.5)), module, OCTA::SNARE_TRIG_LIGHT));
        addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(77.75, 95.5)), module, OCTA::SNARE_TRIG_LIGHT_G));
        addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(82.5, 95.5)), module, OCTA::HHC_TRIG_LIGHT));
        addChild(createLightCentered<MediumLight<WhiteLight>>(mm2px(Vec(87.25, 95.5)), module, OCTA::HHO_TRIG_LIGHT));

        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(18.25, 54.75)), module, OCTA::DRV_KICK_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(41.25, 54.75)), module, OCTA::DRV_SNARE_LIGHT));
        addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(41.25, 54.75)), module, OCTA::DRV_SNARE_LIGHT_G));
        addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(64.25, 54.75)), module, OCTA::DRV_HHC_LIGHT));
        addChild(createLightCentered<MediumLight<WhiteLight>>(mm2px(Vec(87.25, 54.75)), module, OCTA::DRV_HHO_LIGHT));
    }
};

Model *modelOCTA = createModel<OCTA, OCTAWidget>("OCTA");