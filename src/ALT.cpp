#include "plugin.hpp"

struct ALT : Module
{

    float finalGainVCA1 = 0.0f, finalGainVCA2 = 0.0f, finalGainVCA3 = 0.0f, finalGainVCA4 = 0.0f;
    float finalOutVCA1 = 0.0f, finalOutVCA2 = 0.0f, finalOutVCA3 = 0.0f, finalOutVCA4 = 0.0f;
    float normalOutVCA1 = 0.0f, normalOutVCA2 = 0.0f, normalOutVCA3 = 0.0f, normalOutVCA4 = 0.0f;

    enum ParamIds
    {
        GAIN1_PARAM,
        GAIN2_PARAM,
        GAIN3_PARAM,
        ATTVER1_PARAM,
        ATTVER2_PARAM,
        ATTVER3_PARAM,
        NUM_PARAMS
    };
    enum InputIds
    {
        VCA1_INPUT,
        VCA2_INPUT,
        VCA3_INPUT,
        VCA4_INPUT,
        CV1_INPUT,
        CV2_INPUT,
        CV3_INPUT,
        CV4_INPUT,
        NUM_INPUTS
    };
    enum OutputIds
    {
        VCA1_OUTPUT,
        VCA2_OUTPUT,
        VCA3_OUTPUT,
        VCA4_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds
    {
        VCA1_LIGHT,
        VCA2_LIGHT,
        VCA3_LIGHT,
        VCA4_LIGHT,
        NUM_LIGHTS
    };

    ALT()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(GAIN1_PARAM, 0.f, 1.f, 0.f, "VCA1 Gain");
        configParam(GAIN2_PARAM, 0.f, 1.f, 0.f, "VCA2 Gain");
        configParam(GAIN3_PARAM, 0.f, 1.f, 0.f, "VCA3 Gain");
        configParam(ATTVER1_PARAM, -1.f, 1.f, 0.f, "VCA1 CV attenuverter");
        configParam(ATTVER2_PARAM, -1.f, 1.f, 0.f, "VCA2 CV attenuverter");
        configParam(ATTVER3_PARAM, -1.f, 1.f, 0.f, "VCA3 CV attenuverter");

        configInput(VCA1_INPUT, "VCA 1");
        configInput(VCA2_INPUT, "VCA 2");
        configInput(VCA3_INPUT, "VCA 3");
        configInput(VCA4_INPUT, "VCA 4");

        configInput(CV1_INPUT, "CV 1");
        configInput(CV2_INPUT, "CV 2");
        configInput(CV3_INPUT, "CV 3");
        configInput(CV4_INPUT, "CV 4");

        configOutput(VCA1_OUTPUT, "VCA 1");
        configOutput(VCA2_OUTPUT, "VCA 2");
        configOutput(VCA3_OUTPUT, "VCA 3");
        configOutput(VCA4_OUTPUT, "VCA 4");
    }

    void process(const ProcessArgs &args) override
    {

        finalGainVCA1 = params[GAIN1_PARAM].getValue() + (params[ATTVER1_PARAM].getValue() * (inputs[CV1_INPUT].getVoltage() / 5.0f));
        finalGainVCA2 = params[GAIN2_PARAM].getValue() + (params[ATTVER2_PARAM].getValue() * (inputs[CV2_INPUT].getVoltage() / 5.0f));
        finalGainVCA3 = params[GAIN3_PARAM].getValue() + (params[ATTVER3_PARAM].getValue() * (inputs[CV3_INPUT].getVoltage() / 5.0f));

        if (inputs[CV4_INPUT].isConnected())
        {
            finalGainVCA4 = inputs[CV4_INPUT].getVoltage() / 5.0f;
        }
        else
        {
            finalGainVCA4 = 1.0f;
        }

        finalGainVCA1 = clamp(finalGainVCA1, 0.0f, 1.0f);
        finalGainVCA2 = clamp(finalGainVCA2, 0.0f, 1.0f);
        finalGainVCA3 = clamp(finalGainVCA3, 0.0f, 1.0f);
        finalGainVCA4 = clamp(finalGainVCA4, 0.0f, 1.0f);

        finalOutVCA1 = inputs[VCA1_INPUT].getVoltage() * finalGainVCA1;
        finalOutVCA2 = inputs[VCA2_INPUT].getVoltage() * finalGainVCA2;
        finalOutVCA3 = inputs[VCA3_INPUT].getVoltage() * finalGainVCA3;
        finalOutVCA4 = inputs[VCA4_INPUT].getVoltage() * finalGainVCA4;

        normalOutVCA4 = finalOutVCA3 + finalOutVCA4;

        outputs[VCA1_OUTPUT].setVoltage(finalOutVCA1);

        //Out 2 normalling
        normalOutVCA2 = finalOutVCA1 + finalOutVCA2;

        if (outputs[VCA1_OUTPUT].isConnected())
        {
            outputs[VCA2_OUTPUT].setVoltage(finalOutVCA2);
        }
        else
        {
            outputs[VCA2_OUTPUT].setVoltage(normalOutVCA2);
        }

        //Out 3 normalling
        if (outputs[VCA2_OUTPUT].isConnected())
        {
            normalOutVCA3 = finalOutVCA3;
        }
        else
        {
            if (outputs[VCA1_OUTPUT].isConnected())
            {
                normalOutVCA3 = finalOutVCA2 + finalOutVCA3;
            }
            else
            {
                normalOutVCA3 = finalOutVCA1 + finalOutVCA2 + finalOutVCA3;
            }
        }
        outputs[VCA3_OUTPUT].setVoltage(normalOutVCA3);

        //Out 4 normalling
        if (outputs[VCA3_OUTPUT].isConnected())
        {
            normalOutVCA4 = finalOutVCA4;
        }
        else
        {
            if (outputs[VCA2_OUTPUT].isConnected())
            {
                normalOutVCA4 = finalOutVCA3 + finalOutVCA4;
            }
            else
            {
                if (outputs[VCA1_OUTPUT].isConnected())
                {
                    normalOutVCA4 = finalOutVCA2 + finalOutVCA3 + finalOutVCA4;
                }
                else
                {
                    normalOutVCA4 = finalOutVCA1 + finalOutVCA2 + finalOutVCA3 + finalOutVCA4;
                }
            }
        }
        outputs[VCA4_OUTPUT].setVoltage(normalOutVCA4);

        lights[VCA1_LIGHT].setSmoothBrightness(finalGainVCA1, 0.01f);
        lights[VCA2_LIGHT].setSmoothBrightness(finalGainVCA2, 0.01f);
        lights[VCA3_LIGHT].setSmoothBrightness(finalGainVCA3, 0.01f);
        lights[VCA4_LIGHT].setSmoothBrightness(finalGainVCA4, 0.01f);
    }
};

struct ALTWidget : ModuleWidget
{
    ALTWidget(ALT *module)
    {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ALT.svg")));

        addChild(createWidget<ScrewSilver>(Vec(14, 2)));
        //addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        //addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(91, 364)));

        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(31.30, 18.6)), module, ALT::GAIN1_PARAM));
        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(9.30, 41.1)), module, ALT::GAIN2_PARAM));
        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(31.30, 63.6)), module, ALT::GAIN3_PARAM));

        addParam(createParamCentered<Trimpot>(mm2px(Vec(7.3, 18.25)), module, ALT::ATTVER1_PARAM));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(33.3, 40.75)), module, ALT::ATTVER2_PARAM));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(7.3, 63.25)), module, ALT::ATTVER3_PARAM));

        addChild(createLightCentered<MediumLight<WhiteLight>>(mm2px(Vec(20.4, 18.6)), module, ALT::VCA1_LIGHT));
        addChild(createLightCentered<MediumLight<WhiteLight>>(mm2px(Vec(20.2, 41.1)), module, ALT::VCA2_LIGHT));
        addChild(createLightCentered<MediumLight<WhiteLight>>(mm2px(Vec(20.4, 63.6)), module, ALT::VCA3_LIGHT));
        addChild(createLightCentered<MediumLight<WhiteLight>>(mm2px(Vec(35.25, 88.75)), module, ALT::VCA4_LIGHT));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5, 82.25)), module, ALT::VCA1_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15, 82.25)), module, ALT::VCA2_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25, 82.25)), module, ALT::VCA3_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(35, 82.25)), module, ALT::VCA4_INPUT));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5, 95.25)), module, ALT::CV1_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15, 95.25)), module, ALT::CV2_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25, 95.25)), module, ALT::CV3_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(35, 95.25)), module, ALT::CV4_INPUT));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.25, 108.5)), module, ALT::VCA1_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.25, 108.5)), module, ALT::VCA2_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(25.25, 108.5)), module, ALT::VCA3_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(35.25, 108.5)), module, ALT::VCA4_OUTPUT));
    }
};

Model *modelALT = createModel<ALT, ALTWidget>("ALT");