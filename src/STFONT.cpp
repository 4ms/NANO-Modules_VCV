#include "plugin.hpp"
#include "NANOComponents.hpp"

// Include custom classes
#include "Resources/SynthTools/filter.hpp"
#include "Resources/SynthTools/shaper.hpp"
#include "Resources/SynthTools/slewLimiter.hpp"

struct STFONT : Module {
    // Filter class definition
    Filter lFilt, rFilt;
    float l_input, r_input;
    float l_output[3], r_output[3];
    float drive;

    enum ParamIds {
        FREQ_PARAM,
        RES_PARAM,
        MODE_PARAM,
        FREQ_SPAN_PARAM,
        RES_SPAN_PARAM,
        MODE_SPAN_PARAM,
        DRIVE_PARAM,
        CVF_PARAM,
        CVR_PARAM,
        CVM_PARAM,
        RES_MODE_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        L_INPUT,
        R_INPUT,
        DRV_INPUT,
        FRQ_INPUT,
        RES_INPUT,
        MOD_INPUT,
        VOCT_INPUT,
        FRQ_SP_INPUT,
        RES_SP_INPUT,
        MOD_SP_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        L_OUTPUT,
        R_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        DRV_L_LIGHT,
        DRV_R_LIGHT,
        NUM_LIGHTS
    };

    STFONT() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(FREQ_PARAM, 0.0f, 1.0f, 0.5f, "Cutoff frequency");
        configParam(RES_PARAM, 0.0f, 1.0f, 0.5f, "Resonance");
        configParam(MODE_PARAM, 0.0f, 1.0f, 0.0f, "Mode");
        configParam(DRIVE_PARAM, 0.0f, 1.0f, 0.5f, "Drive");

        configParam(FREQ_SPAN_PARAM, -1.0f, 1.0f, 0.f, "Frequency Span");
        configParam(RES_SPAN_PARAM, -1.0f, 1.0f, 0.f, "Resonance Span");
        configParam(MODE_SPAN_PARAM, -1.0f, 1.0f, 0.f, "Mode Span");

        configParam(CVF_PARAM, -1.f, 1.f, 0.f, "Cutoff attenuverter");
        configParam(CVR_PARAM, -1.f, 1.f, 0.f, "Resonance attenuverter");
        configParam(CVM_PARAM, -1.f, 1.f, 0.f, "Mode attenuverter");

        configParam(RES_MODE_PARAM, 0.f, 1.f, 0.f, "Resonance mode");

        configInput(FRQ_INPUT, "CV Frequency");
        configInput(VOCT_INPUT, "CV V/OCT");
        configInput(RES_INPUT, "CV Resonance");

        configInput(L_INPUT, "Left");
        configInput(R_INPUT, "Right");
        configOutput(L_OUTPUT, "Left");
        configOutput(R_OUTPUT, "Right");

        lFilt.init();
        rFilt.init();
    }

    void process(const ProcessArgs &args) override {
        if (inputs[DRV_INPUT].isConnected()) {
            drive = std::max(0.0f, inputs[DRV_INPUT].getVoltage() * Shaper::mapToRange(params[DRIVE_PARAM].getValue(), 0.0f, 1.0f, 0.0f, 0.5f, 2.0f) / 5.0f);
        } else {
            drive = Shaper::mapToRange(params[DRIVE_PARAM].getValue(), 0.0f, 1.0f, 0.0f, 0.75f, 2.0f);
        }

        l_input = Shaper::softClip((inputs[L_INPUT].getVoltage() * drive), 7.0f);

        if (inputs[R_INPUT].isConnected()) {
            r_input = Shaper::softClip((inputs[R_INPUT].getVoltage() * drive), 7.0f);
        } else {
            r_input = l_input;
        }

        // Get the base frequency cutoff value from the frequency parameter
        float baseCutoff = params[FREQ_PARAM].getValue() - 0.01f;

        // Calculate the modulation from the CV input and CV frequency parameter
        float cvfModulation = inputs[FRQ_INPUT].getVoltage() * params[CVF_PARAM].getValue() / 7.0f;

        // Adjust the cutoff frequency by adding the modulation
        float l_cutoff = baseCutoff + cvfModulation;

        // Map the cutoff to a curved range and add V/OCT input
        l_cutoff = Shaper::shapeCurve(l_cutoff, 0.0001, 1.0f, 0.01f, 0.3f) + (inputs[VOCT_INPUT].getVoltage() / 15.0f);

        // Get the base resonance value from the resonance parameter
        float baseResonance = params[RES_PARAM].getValue();

        // Calculate the modulation from the CV input and CV resonance parameter
        float cvrModulation = (inputs[RES_INPUT].getVoltage() * params[CVR_PARAM].getValue()) / 5.0f;

        // Adjust the resonance by adding the modulation
        float l_resonance = baseResonance + cvrModulation;

        if (params[RES_MODE_PARAM].getValue()) {
            lFilt.setResonanceMode(1);
            rFilt.setResonanceMode(1);
        } else {
            lFilt.setResonanceMode(0);
            rFilt.setResonanceMode(0);
        }

        // Get the base resonance value from the resonance parameter
        float baseMode = params[MODE_PARAM].getValue();

        // Calculate the modulation from the CV input and CV resonance parameter
        float cvmModulation = (inputs[MOD_INPUT].getVoltage() * params[CVM_PARAM].getValue()) / 5.0f;

        // Adjust the resonance by adding the modulation
        float l_mode = clamp(baseMode + cvmModulation, 0.0f, 1.0f);

        float frq_span = Shaper::mapToRange(params[FREQ_SPAN_PARAM].getValue() + inputs[FRQ_SP_INPUT].getVoltage() / 5.0f, -1.0f, 1.0f, 0.025f, 1.0f, 40.0f);
        float res_span = params[RES_SPAN_PARAM].getValue() + inputs[RES_SP_INPUT].getVoltage() / 5.0f;
        float mod_span = params[MODE_SPAN_PARAM].getValue() + inputs[MOD_SP_INPUT].getVoltage() / 5.0f;

        float r_cutoff = l_cutoff * frq_span;
        float r_resonance = clamp(l_resonance + res_span, 0.0f, 1.0f);
        float r_mode = clamp(l_mode + mod_span, 0.0f, 1.0f);

        lFilt.setCutoff(l_cutoff);
        rFilt.setCutoff(r_cutoff);

        lFilt.setResonance(l_resonance);
        rFilt.setResonance(r_resonance);

        lFilt.setInput(l_input);
        rFilt.setInput(r_input);

        lFilt.process();
        rFilt.process();

        l_output[0] = lFilt.getLowPass();
        l_output[1] = lFilt.getBandPass();
        l_output[2] = lFilt.getHighPass();

        r_output[0] = rFilt.getLowPass();
        r_output[1] = rFilt.getBandPass();
        r_output[2] = rFilt.getHighPass();

        float l_master = Shaper::multiInputCrossfade(l_output, 3, l_mode);
        float r_master = Shaper::multiInputCrossfade(r_output, 3, r_mode);

        outputs[L_OUTPUT].setVoltage(l_master);
        outputs[R_OUTPUT].setVoltage(r_master);

        if (l_input > 5.0f)
            lights[DRV_L_LIGHT].setSmoothBrightness(1.0f, 0.00001f);
        else
            lights[DRV_L_LIGHT].setSmoothBrightness(0.0f, 0.00001f);

        if (r_input > 5.0f)
            lights[DRV_R_LIGHT].setSmoothBrightness(1.0f, 0.00001f);
        else
            lights[DRV_R_LIGHT].setSmoothBrightness(0.0f, 0.00001f);
    }
};

struct STFONTWidget : ModuleWidget {
    STFONTWidget(STFONT *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/STFONT.svg")));

        addChild(createWidget<ScrewSilver>(Vec(13.44, 1.5)));
        addChild(createWidget<ScrewSilver>(Vec(13.44, 363)));
        addChild(createWidget<ScrewSilver>(Vec(180.94, 1.5)));
        addChild(createWidget<ScrewSilver>(Vec(180.94, 363)));

        addParam(createParamCentered<Davies1900hLargeBlackKnob>(mm2px(Vec(16.44, 18.25)), module, STFONT::FREQ_PARAM));
        addParam(createParamCentered<Davies1900hLargeBlackKnob>(mm2px(Vec(54.44, 18.25)), module, STFONT::MODE_PARAM));
        addParam(createParamCentered<Davies1900hBlackKnob>(mm2px(Vec(35.44, 35.0)), module, STFONT::RES_PARAM));
        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(12.44, 45.0)), module, STFONT::FREQ_SPAN_PARAM));
        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(35.44, 56.75)), module, STFONT::RES_SPAN_PARAM));
        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(58.44, 45.0)), module, STFONT::MODE_SPAN_PARAM));
        addParam(createParamCentered<Davies1900hBlackKnob>(mm2px(Vec(12.44, 77.0)), module, STFONT::DRIVE_PARAM));

        addParam(createParamCentered<NANOComponents::BarkSwitchSmallSide2P>(mm2px(Vec(55.565, 64.125)), module, STFONT::RES_MODE_PARAM));

        addParam(createParamCentered<Trimpot>(mm2px(Vec(30.815, 77.0)), module, STFONT::CVF_PARAM));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(46.815, 77.0)), module, STFONT::CVR_PARAM));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(62.815, 77.0)), module, STFONT::CVM_PARAM));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.94, 95.25)), module, STFONT::L_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(18.94, 95.25)), module, STFONT::R_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(29.94, 95.25)), module, STFONT::DRV_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(40.94, 95.25)), module, STFONT::FRQ_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(51.94, 95.25)), module, STFONT::RES_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(62.94, 95.25)), module, STFONT::MOD_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.94, 108.25)), module, STFONT::L_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(18.94, 108.25)), module, STFONT::R_OUTPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(29.94, 108.25)), module, STFONT::VOCT_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(40.94, 108.25)), module, STFONT::FRQ_SP_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(51.94, 108.25)), module, STFONT::RES_SP_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(62.94, 108.25)), module, STFONT::MOD_SP_INPUT));

        addChild(createLightCentered<MediumLight<WhiteLight>>(mm2px(Vec(5.94, 62.5)), module, STFONT::DRV_L_LIGHT));
        addChild(createLightCentered<MediumLight<WhiteLight>>(mm2px(Vec(18.94, 62.5)), module, STFONT::DRV_R_LIGHT));
    }
};

Model *modelSTFONT = createModel<STFONT, STFONTWidget>("STFONT");