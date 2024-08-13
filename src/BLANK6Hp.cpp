#include "plugin.hpp"
#include <componentlibrary.hpp>
#include "NANOComponents.hpp"

struct BLANK6Hp : Module
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

    BLANK6Hp()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    }

    void process(const ProcessArgs &args) override{

    };
};

struct BLANK6HpWidget : ModuleWidget
{
    BLANK6HpWidget(BLANK6Hp *module)
    {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BLANK6Hp.svg")));

        addChild(createWidget<ScrewSilver>(Vec(14, 1.5)));
        //addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        //addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(60.5, 363.5)));
    }
};

Model *modelBLANK6Hp = createModel<BLANK6Hp, BLANK6HpWidget>("BLANK6Hp");