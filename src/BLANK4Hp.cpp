#include "plugin.hpp"
#include <componentlibrary.hpp>
#include "NANOComponents.hpp"

struct BLANK4Hp : Module
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

    BLANK4Hp()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    }

    void process(const ProcessArgs &args) override{

    };
};

struct BLANK4HpWidget : ModuleWidget
{
    BLANK4HpWidget(BLANK4Hp *module)
    {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BLANK4Hp.svg")));

        addChild(createWidget<ScrewSilver>(Vec(13.5, 1.5)));
        //addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        //addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(30.2, 363.5)));
    }
};

Model *modelBLANK4Hp = createModel<BLANK4Hp, BLANK4HpWidget>("BLANK4Hp");