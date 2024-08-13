#include "plugin.hpp"
#include <componentlibrary.hpp>
#include "NANOComponents.hpp"

#include "Resources/DaisySP/Source/Filters/svf.h"
#include "Resources/DaisySP/Source/Dynamics/compressor.h"

struct CEQ : Module
{   
    float l_input, r_input;

    enum ParamIds
    {
        HIGH_PARAM,
        MID_PARAM,
        LOW_PARAM,
        COMP_PARAM,
        NUM_PARAMS
    };
    enum InputIds
    {
        L_INPUT,
        R_INPUT,
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
		NUM_LIGHTS
	};

    // Create svf filter classes
    daisysp::Svf l_low, l_high;
    daisysp::Svf r_low, r_high;
    // Create compresor classes
    daisysp::Compressor l_comp, r_comp;

    CEQ()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        // Config parameter ranges
        configParam(HIGH_PARAM, 0.f, 2.f, 1.0f, "High frequency gain");
        configParam(MID_PARAM, 0.f, 2.f, 1.0f, "Mid frequency gain");
        configParam(LOW_PARAM, 0.f, 2.f, 1.0f, "Low frequency gain");
        configParam(COMP_PARAM, 0.f, 1.f, 0.0f, "Compression amount");

        configInput(L_INPUT, "L");
        configInput(R_INPUT, "R");

        configOutput(L_OUTPUT, "L");
        configOutput(R_OUTPUT, "R");

        // Initialize filter classes
        l_low.Init(44100.0f);
        r_low.Init(44100.0f);
        l_high.Init(44100.0f);
        r_high.Init(44100.0f);

        // Set cutoff frequencies for the filters
        l_low.SetFreq(256.7f);
        r_low.SetFreq(256.7f);
        l_high.SetFreq(2567.0f);
        r_high.SetFreq(2567.0f);

        // Initialize compressor classes
        l_comp.Init(44100.0f);
        r_comp.Init(44100.0f);
    }

    void process(const ProcessArgs &args) override
    {   
        // Read voltage inputs
        l_input = inputs[L_INPUT].getVoltage();

        //Do normalization if required
        if(inputs[R_INPUT].isConnected()){
            r_input = inputs[R_INPUT].getVoltage();
        } else {
            r_input = l_input;
        }

        // Reduce and clamp the filter inputs to avoid clipping and failure
        float l_clamped = clamp(l_input / 4.0f, -4.0f, 4.0f);
        float r_clamped = clamp(r_input / 4.0f, -4.0f, 4.0f);

        // Process filter outputs
        l_low.Process(l_clamped);
        r_low.Process(r_clamped);
        l_high.Process(l_clamped);
        r_high.Process(r_clamped);

        // Get bandpass outputs
        float band_l = (l_low.High() - l_high.High());
        float band_r = (r_low.High() - r_high.High());

        // Summ all the bands together depending on the parameter settings
        float mix_l = (l_low.Low() * params[LOW_PARAM].getValue()) + (band_l * params[MID_PARAM].getValue()) + (l_high.High() * params[HIGH_PARAM].getValue());
        float mix_r = (r_low.Low() * params[LOW_PARAM].getValue()) + (band_r * params[MID_PARAM].getValue()) + (r_high.High() * params[HIGH_PARAM].getValue());

        // Set the compressor variables depending on the parameter setting
        float comp_thres = params[COMP_PARAM].getValue() * -10.0f;
        float comp_ratio = params[COMP_PARAM].getValue() * 4.0f + 1.0f;
        float comp_makeup = params[COMP_PARAM].getValue() * 14.0f;

        // Write variables to the compressor classes
        l_comp.SetAttack(0.01f);
        l_comp.SetRelease(0.01f);
        l_comp.SetRatio(comp_ratio);
        l_comp.SetThreshold(comp_thres);
        l_comp.SetMakeup(comp_makeup);

        r_comp.SetAttack(0.01f);
        r_comp.SetRelease(0.01f);
        r_comp.SetRatio(comp_ratio);
        r_comp.SetThreshold(comp_thres);
        r_comp.SetMakeup(comp_makeup);

        // Get the compressed signal
        float final_l = l_comp.Process(mix_l * 4.0f);
        float final_r = r_comp.Process(mix_r * 4.0f);

        // Clamp to eurorack voltage levels to match the hardware module
        float final_clamp_l = clamp(final_l, -11.0f, 11.0f);
        float final_clamp_r = clamp(final_r, -11.0f, 11.0f);
        
        // Write the output voltages
        outputs[L_OUTPUT].setVoltage(final_clamp_l);
        outputs[R_OUTPUT].setVoltage(final_clamp_r);

        // Set the LEDs brightness depending on output signal
        lights[L_LIGHT].setBrightness(final_clamp_l);
        lights[R_LIGHT].setBrightness(final_clamp_r);
    }
};

struct CEQWidget : ModuleWidget
{
    CEQWidget(CEQ *module)
    {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/CEQ.svg")));

        addChild(createWidget<ScrewSilver>(Vec(14, 1.5)));
        //addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        //addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(30, 363.5)));

        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(10.0, 44.25)), module, CEQ::HIGH_PARAM));
        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(10.0, 64.75)), module, CEQ::MID_PARAM));
        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(10.0, 85.25)), module, CEQ::LOW_PARAM));
        addParam(createParamCentered<Davies1900hBlackKnob>(mm2px(Vec(10.0, 105.75)), module, CEQ::COMP_PARAM));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5,  14.25)), module, CEQ::L_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15, 14.25)), module, CEQ::R_INPUT));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5, 27.25)), module, CEQ::L_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15, 27.25)), module, CEQ::R_OUTPUT));

        addChild(createLightCentered<SmallLight<WhiteLight>>(mm2px(Vec(3.25, 115)), module, CEQ::L_LIGHT));
        addChild(createLightCentered<SmallLight<WhiteLight>>(mm2px(Vec(16.75, 115)), module, CEQ::R_LIGHT));
    }
};

Model *modelCEQ = createModel<CEQ, CEQWidget>("CEQ");