#include "plugin.hpp"

struct FONT : Module
{

    float firstStage = 0.f, secondStage = 0.f;
    float resonanceFactor = 1.0f;

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
        CVF_INPUT,
        OCT_INPUT,
        CVR_INPUT,
        IN_INPUT,
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
        configParam(RES_PARAM, 0.0f, 28.0f, 0.f, "Resonance");
        configParam(CVF_PARAM, -1.f, 1.f, 0.f, "Cutoff attenuverter");
        configParam(CVR_PARAM, -1.f, 1.f, 0.f, "Resonance attenuverter");

        configInput(CVF_INPUT, "CV Frequency");
        configInput(OCT_INPUT, "CV V/OCT");
        configInput(CVR_INPUT, "CV Resonance");

        configInput(IN_INPUT, "Filter");
        configOutput(LPF_OUTPUT, "Low Pass");
        configOutput(BPF_OUTPUT, "Band Pass");
    }

    void process(const ProcessArgs &args) override
    {

        float cutoff =
            params[FREQ_PARAM].getValue() - 0.01f +
            (inputs[CVF_INPUT].getVoltage() * params[CVF_PARAM].getValue()) / 10.0f;

        cutoff += inputs[OCT_INPUT].getVoltage() / 15.0f;

        cutoff = clamp(cutoff, 0.001f, 0.999999999f);

        float resonance = params[RES_PARAM].getValue() + (inputs[CVR_INPUT].getVoltage() * params[CVR_PARAM].getValue() * 5.0f);

        resonance = clamp(resonance, 0.0f, 28.0f);

        //Resonance feeding
        if (resonance >= 20)
        {
            firstStage += 0.001f;
        }

        firstStage += cutoff * (inputs[IN_INPUT].getVoltage() - firstStage + (resonance * (tanh((firstStage - secondStage) / 10.0f))));
        //firstStage = clamp(firstStage, -10.0f, 10.0f);

        secondStage += cutoff * (firstStage - secondStage + 0.1f);
        //secondStage = clamp(secondStage, -10.0f, 10.0f);

        outputs[LPF_OUTPUT].setVoltage(tanh(secondStage / 10.0f) * 11.0f);
        outputs[BPF_OUTPUT].setVoltage(tanh((firstStage - secondStage) / 10.0f) * 11.0f);
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
        addChild(createWidget<ScrewSilver>(Vec(60.5, 363)));

        addParam(createParamCentered<Davies1900hLargeBlackKnob>(mm2px(Vec(15, 18.25)), module, FONT::FREQ_PARAM));
        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(15, 44.75)), module, FONT::RES_PARAM));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(6, 69.75)), module, FONT::CVF_PARAM));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(24, 69.75)), module, FONT::CVR_PARAM));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5, 95.25)), module, FONT::CVF_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15, 95.25)), module, FONT::OCT_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25, 95.25)), module, FONT::CVR_INPUT));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5, 108.25)), module, FONT::IN_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15, 108.25)), module, FONT::LPF_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(25, 108.25)), module, FONT::BPF_OUTPUT));
    }
};

Model *modelFONT = createModel<FONT, FONTWidget>("FONT");