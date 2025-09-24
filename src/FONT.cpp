#include "plugin.hpp"

// Include custom filter class
#include "Resources/SynthTools/filter.hpp"
#include "Resources/SynthTools/shaper.hpp"

struct FONT : Module
{
    // Filter class definition
    Filter filter;

    enum ParamIds
    {
        FREQ_PARAM,
        RES_PARAM,
        CVF_PARAM,
        CVR_PARAM,
        NUM_PARAMS
    };
    enum InputIds
    {
        FRQ_INPUT,
        OCT_INPUT,
        RES_INPUT,
        FILT_INPUT,
        NUM_INPUTS,
    };
    enum OutputIds
    {
        LPF_OUTPUT,
        BPF_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds
    {
        NUM_LIGHTS
    };

    FONT()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(FREQ_PARAM, 0.0f, 1.0f, 0.f, "Cutoff frequency");
        configParam(RES_PARAM, 0.0f, 1.0f, 0.f, "Resonance");
        configParam(CVF_PARAM, -1.f, 1.f, 0.f, "Cutoff attenuverter");
        configParam(CVR_PARAM, -1.f, 1.f, 0.f, "Resonance attenuverter");

        configInput(FRQ_INPUT, "CV Frequency");
        configInput(OCT_INPUT, "CV V/OCT");
        configInput(RES_INPUT, "CV Resonance");

        configInput(FILT_INPUT, "Filter");
        configOutput(LPF_OUTPUT, "Low Pass");
        configOutput(BPF_OUTPUT, "Band Pass");

        filter.init();
    }

    void process(const ProcessArgs &args) override
    {
        // Get the base frequency cutoff value from the frequency parameter
        float baseCutoff = params[FREQ_PARAM].getValue() - 0.01f;

        // Calculate the modulation from the CV input and CV frequency parameter
        float cvfModulation = (inputs[FRQ_INPUT].getVoltage() * params[CVF_PARAM].getValue()) / 10.0f;

        // Adjust the cutoff frequency by adding the modulation
        float cutoff = baseCutoff + cvfModulation;

        // Map the cutoff to a curved range and add V/OCT input
        cutoff = Shaper::shapeCurve(cutoff, 0.0001, 1.0f, 0.01f, 0.3f) + inputs[OCT_INPUT].getVoltage() / 15.0f;

        // Get the base resonance value from the resonance parameter
        float baseResonance = params[RES_PARAM].getValue();

        // Calculate the modulation from the CV input and CV resonance parameter
        float cvrModulation = (inputs[RES_INPUT].getVoltage() * params[CVR_PARAM].getValue()) / 5.0f;

        // Adjust the resonance by adding the modulation
        float resonance = baseResonance + cvrModulation;

        filter.setCutoff(cutoff);

        filter.setResonance(resonance);

        filter.setInput(inputs[FILT_INPUT].getVoltage());

        filter.process();

        outputs[LPF_OUTPUT].setVoltage(filter.getLowPass());
        outputs[BPF_OUTPUT].setVoltage(filter.getBandPass());
    }
};

struct FONTWidget : ModuleWidget
{
    FONTWidget(FONT *module)
    {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/FONT.svg")));

        addChild(createWidget<ScrewSilver>(Vec(14, 1.5)));
        //addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        //addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(61.5, 363)));

        addParam(createParamCentered<Davies1900hLargeBlackKnob>(mm2px(Vec(15.35, 18.25)), module, FONT::FREQ_PARAM));
        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(15.35, 44.75)), module, FONT::RES_PARAM));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(6.35, 69.75)), module, FONT::CVF_PARAM));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(24.35, 69.75)), module, FONT::CVR_PARAM));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.35, 95.25)), module, FONT::FRQ_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.35, 95.25)), module, FONT::OCT_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25.35, 95.25)), module, FONT::RES_INPUT));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.35, 108.25)), module, FONT::FILT_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.35, 108.25)), module, FONT::LPF_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(25.35, 108.25)), module, FONT::BPF_OUTPUT));
    }
};

Model *modelFONT = createModel<FONT, FONTWidget>("FONT");