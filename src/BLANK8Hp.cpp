#include "plugin.hpp"
#include <componentlibrary.hpp>
#include "NANOComponents.hpp"

struct BLANK8Hp : Module
{

    enum ParamIds
    {
        NUM_PARAMS
    };
    enum InputIds
    {
        NUM_INPUTS
    };
    enum OutputIds
    {
        NUM_OUTPUTS
    };
    enum LightIds
    {
        NUM_LIGHTS
    };

    BLANK8Hp()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    }

    void process(const ProcessArgs &args) override{

    };
};

struct BLANK8HpWidget : ModuleWidget
{
    BLANK8HpWidget(BLANK8Hp *module)
    {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BLANK8Hp.svg")));

        addChild(createWidget<ScrewSilver>(Vec(14, 1.5)));
        //addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        //addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(90, 363.5)));
    }
};

Model *modelBLANK8Hp = createModel<BLANK8Hp, BLANK8HpWidget>("BLANK8Hp");