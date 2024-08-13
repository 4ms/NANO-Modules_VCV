#include "plugin.hpp"
#include <componentlibrary.hpp>
#include "NANOComponents.hpp"

struct BLANK2Hp : Module
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

    BLANK2Hp()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    }

    void process(const ProcessArgs &args) override{

    };
};

struct BLANK2HpWidget : ModuleWidget
{
    BLANK2HpWidget(BLANK2Hp *module)
    {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BLANK2Hp.svg")));

        addChild(createWidget<ScrewSilver>(Vec(8, 1.5)));
        //addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        //addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(8, 363.5)));
    }
};

Model *modelBLANK2Hp = createModel<BLANK2Hp, BLANK2HpWidget>("BLANK2Hp");