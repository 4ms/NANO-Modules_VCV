#include "plugin.hpp"
#include <componentlibrary.hpp>
#include "NANOComponents.hpp"
#include "PerformanceMixer.hpp"

struct EXP4 : Module
{       
    float l_exp4[MIXER_CHANNELS] = {0.0f, 0.0f, 0.0f, 0.0f};
    float r_exp4[MIXER_CHANNELS] = {0.0f, 0.0f, 0.0f, 0.0f};

    enum ParamIds
    {
        NUM_PARAMS
    };
    enum InputIds
    {
        MUTE_GATE_1,
        MUTE_GATE_2,
        MUTE_GATE_3,
        MUTE_GATE_4,
        CV_AUX_3, 
        CV_AUX_4,             
        NUM_INPUTS
    };
    enum OutputIds
    {
        L1_OUTPUT,
        L2_OUTPUT,
        L3_OUTPUT,       
        L4_OUTPUT,
        R1_OUTPUT,
        R2_OUTPUT,
        R3_OUTPUT, 
        R4_OUTPUT,
        NUM_OUTPUTS
    };

    EXP4()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);

        configOutput(L1_OUTPUT, "L1");
        configOutput(R1_OUTPUT, "R1");
        configOutput(L2_OUTPUT, "L2");
        configOutput(R2_OUTPUT, "R2");
        configOutput(L3_OUTPUT, "L3");
        configOutput(R3_OUTPUT, "R3");
        configOutput(L4_OUTPUT, "L4");
        configOutput(R4_OUTPUT, "R4");

        configInput(MUTE_GATE_1, "Mute Gate 1");
        configInput(MUTE_GATE_2, "Mute Gate 2");
        configInput(MUTE_GATE_3, "Mute Gate 3");
        configInput(MUTE_GATE_4, "Mute Gate 4");

        configInput(CV_AUX_3, "CV Aux 3");
        configInput(CV_AUX_4, "CV Aux 4");
    }

    void process(const ProcessArgs &args) override
    {
        // Check if the main module is on the right
        bool mainModuleConnected = rightExpander.module && (rightExpander.module->model == modelPerformanceMixer);

        if (mainModuleConnected) {
            // Access the shared data
            SharedData* sharedData = (SharedData*) rightExpander.module->leftExpander.producerMessage;

            if (sharedData) {
                for(uint32_t i = 0; i < MIXER_CHANNELS; i++){
                    // Read the shared data
                    l_exp4[i] = sharedData->shared_l_output[i];
                    r_exp4[i] = sharedData->shared_r_output[i];
                    if(inputs[MUTE_GATE_1 + i].getVoltage() >= 2.0f){
                        sharedData->shared_gateMuted[i] = true;
                    } else {
                        sharedData->shared_gateMuted[i] = false;
                    }
                }

                sharedData->shared_cv_aux[2] = inputs[CV_AUX_3].getVoltage();
                sharedData->shared_cv_aux[3] = inputs[CV_AUX_4].getVoltage();

                // Do something with the value
                // ...
            }
        }
    
        // Write voltage outputs
        for(uint32_t i = 0; i < MIXER_CHANNELS; i++){
            outputs[L1_OUTPUT + i].setVoltage(l_exp4[i]);
            outputs[R1_OUTPUT + i].setVoltage(r_exp4[i]);
        }
    }

};

struct EXP4Widget : ModuleWidget
{
    EXP4Widget(EXP4 *module)
    {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/EXP4.svg")));

        addChild(createWidget<ScrewSilver>(Vec(14, 1.5)));
        //addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        //addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(32.5, 363.5)));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5 ,  14.50)), module, EXP4::L1_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15,  14.50)), module, EXP4::R1_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5 ,  27.50)), module, EXP4::L2_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15,  27.50)), module, EXP4::R2_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5 ,  40.50)), module, EXP4::L3_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15,  40.50)), module, EXP4::R3_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5 ,  53.50)), module, EXP4::L4_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15,  53.50)), module, EXP4::R4_OUTPUT));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5 ,  75.00)), module, EXP4::MUTE_GATE_1));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15,  75.00)), module, EXP4::MUTE_GATE_2));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5 ,  88.00)), module, EXP4::MUTE_GATE_3));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15,  88.00)), module, EXP4::MUTE_GATE_4));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5 , 108.00)), module, EXP4::CV_AUX_3));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15, 108.00)), module, EXP4::CV_AUX_4));
    }
};

Model *modelEXP4 = createModel<EXP4, EXP4Widget>("EXP4");