#include "plugin.hpp"
#include <componentlibrary.hpp>
#include "NANOComponents.hpp"
#include "PerformanceMixer.hpp"

#define LED_SMOOTHING 0.00005f
#define SLEW_SMOOTHING 0.005f
#define MIXER_CHANNELS 4
#define MIXER_AUX 2

struct PerformanceMixer : Module
{   
    float pot_vol[MIXER_CHANNELS] = {0.0f, 0.0f, 0.0f, 0.0f};
    float pot_pan[MIXER_CHANNELS] = {0.0f, 0.0f, 0.0f, 0.0f};
    float pot_aux[MIXER_CHANNELS] = {0.0f, 0.0f, 0.0f, 0.0f};
    float l_input[MIXER_CHANNELS] = {0.0f, 0.0f, 0.0f, 0.0f};
    float r_input[MIXER_CHANNELS] = {0.0f, 0.0f, 0.0f, 0.0f};
    float l_output[MIXER_CHANNELS] = {0.0f, 0.0f, 0.0f, 0.0f};
    float r_output[MIXER_CHANNELS] = {0.0f, 0.0f, 0.0f, 0.0f};
    float cv_aux[MIXER_CHANNELS] = {0.0f, 0.0f, 0.0f, 0.0f};
    float cv_vol[MIXER_CHANNELS] = {0.0f, 0.0f, 0.0f, 0.0f};
    float cv_pan[MIXER_CHANNELS] = {0.0f, 0.0f, 0.0f, 0.0f};
    float slewMute[MIXER_CHANNELS] = {0.0f, 0.0f, 0.0f, 0.0f};
    float slewCue[MIXER_CHANNELS] = {0.0f, 0.0f, 0.0f, 0.0f};
    bool isCued[MIXER_CHANNELS] = {false, false, false, false};
    bool isMuted[MIXER_CHANNELS] = {false, false, false, false};
    bool isGateMuted[MIXER_CHANNELS] = {false, false, false, false};
    bool isFinallyMuted[MIXER_CHANNELS] = {false, false, false, false};
    bool isPressed[MIXER_CHANNELS] = {false, false, false, false};    
    bool wasPressed[MIXER_CHANNELS] = {false, false, false, false};
    bool sendtoX[MIXER_CHANNELS] = {false, false, false, false};
    bool prePost[MIXER_AUX] = {false, false};
    float gain_ret[MIXER_AUX] = {0.0f, 0.0f};
    float aux_ret_l[MIXER_AUX] = {0.0f, 0.0f};
    float aux_ret_r[MIXER_AUX] = {0.0f, 0.0f};
    float mono_in[MIXER_CHANNELS] = {0.0f, 0.0f, 0.0f, 0.0f};
    float gain_pre[MIXER_CHANNELS] = {0.0f, 0.0f, 0.0f, 0.0f};
    float gain_L[MIXER_CHANNELS] = {0.0f, 0.0f, 0.0f, 0.0f};
    float gain_R[MIXER_CHANNELS] = {0.0f, 0.0f, 0.0f, 0.0f};
    float pan_pre[MIXER_CHANNELS] = {0.0f, 0.0f, 0.0f, 0.0f};
    float aux_pre[MIXER_CHANNELS] = {0.0f, 0.0f, 0.0f, 0.0f};
    
    float mix_ret_l = 0.0f, mix_ret_r = 0.0f;
    float x_send = 0.0f, y_send = 0.0f;
    float mix_l = 0.0f, mix_r = 0.0f;
    float mix_cue = 0.0f;

    enum ParamIds
    {
        AUX1_PARAM,
        AUX2_PARAM,
        AUX3_PARAM,
        AUX4_PARAM,
        PAN1_PARAM,
        PAN2_PARAM,
        PAN3_PARAM,
        PAN4_PARAM,
        VOL1_PARAM,
        VOL2_PARAM,
        VOL3_PARAM,
        VOL4_PARAM,
        MUTE1_PARAM,
        MUTE2_PARAM,
        MUTE3_PARAM,
        MUTE4_PARAM,
        CUE1_PARAM,
        CUE2_PARAM,
        CUE3_PARAM,
        CUE4_PARAM,
        PRE_X_PARAM,
        PRE_Y_PARAM,
        PHONES_VOL_PARAM,
        PHONES_MIX_PARAM,
        AUX_X_VOL_PARAM,
        AUX_Y_VOL_PARAM,
        MASTER_VOL_PARAM,
        NUM_PARAMS
    };
    enum InputIds
    {
        L1_INPUT,
        L2_INPUT,
        L3_INPUT,       
        L4_INPUT,
        R1_INPUT,
        R2_INPUT,
        R3_INPUT, 
        R4_INPUT,
        AUX_X_L_INPUT,
        AUX_Y_L_INPUT,
        AUX_X_R_INPUT,
        AUX_Y_R_INPUT,
        CV_VOL_1,
        CV_VOL_2,
        CV_VOL_3,
        CV_VOL_4,       
        CV_PAN_1,
        CV_PAN_2,
        CV_PAN_3,
        CV_PAN_4,    
        CV_AUX_1,
        CV_AUX_2, 
        CV_AUX_3, 
        CV_AUX_4,             
        NUM_INPUTS
    };
    enum OutputIds
    {
        L_OUTPUT,
        R_OUTPUT,
        X_AUX_OUTPUT,
        Y_AUX_OUTPUT,
        CUE_OUTPUT,
        PHONES_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        VOL1_LIGHT,
        VOL2_LIGHT,
        VOL3_LIGHT,
        VOL4_LIGHT,
        MUTE1_LIGHT,
        MUTE2_LIGHT,
        MUTE3_LIGHT,
        MUTE4_LIGHT,
        L_LIGHT,
        R_LIGHT,
        CLIPL_LIGHT,
        CLIPR_LIGHT,
		NUM_LIGHTS
	};

    SharedData sharedData;

    PerformanceMixer()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        // Config parameter ranges
        configParam(AUX1_PARAM, -1.0f, 1.f, 0.0f, "Channel 1 AUX Send");
        configParam(AUX2_PARAM, -1.0f, 1.f, 0.0f, "Channel 2 AUX Send");
        configParam(AUX3_PARAM, -1.0f, 1.f, 0.0f, "Channel 3 AUX Send");
        configParam(AUX4_PARAM, -1.0f, 1.f, 0.0f, "Channel 4 AUX Send");
        
        configParam(PAN1_PARAM, 0.0f, 1.f, 0.5f, "Channel 1 Panning");
        configParam(PAN2_PARAM, 0.0f, 1.f, 0.5f, "Channel 2 Panning");
        configParam(PAN3_PARAM, 0.0f, 1.f, 0.5f, "Channel 3 Panning");
        configParam(PAN4_PARAM, 0.0f, 1.f, 0.5f, "Channel 4 Panning");

        configParam(VOL1_PARAM, 0.0f, 1.f, 0.75f, "Channel 1 Volume");
        configParam(VOL2_PARAM, 0.0f, 1.f, 0.75f, "Channel 2 Volume");
        configParam(VOL3_PARAM, 0.0f, 1.f, 0.75f, "Channel 3 Volume");
        configParam(VOL4_PARAM, 0.0f, 1.f, 0.75f, "Channel 4 Volume");

        configParam(AUX_X_VOL_PARAM, 0.0f, 1.f, 0.0f, "X Return Volume");
        configParam(AUX_Y_VOL_PARAM, 0.0f, 1.f, 0.0f, "Y Return Volume");

        configParam(PHONES_VOL_PARAM, 0.0f, 1.f, 0.0f, "Phones Volume");
        configParam(PHONES_MIX_PARAM, 0.0f, 1.f, 0.0f, "Phones Source Mix");
        configParam(MASTER_VOL_PARAM, 0.0f, 1.f, 0.0f, "Master Volume");

        configParam(MUTE1_PARAM, 0.0f, 1.f, 0.0f, "Channel 1 Mute");
        configParam(MUTE2_PARAM, 0.0f, 1.f, 0.0f, "Channel 2 Mute");
        configParam(MUTE3_PARAM, 0.0f, 1.f, 0.0f, "Channel 3 Mute");
        configParam(MUTE4_PARAM, 0.0f, 1.f, 0.0f, "Channel 4 Mute");

        configSwitch(CUE1_PARAM, 0.0f, 1.f, 1.0f, "Channel 1 Cue Switch", {"ON", "OFF"});
        configSwitch(CUE2_PARAM, 0.0f, 1.f, 1.0f, "Channel 2 Cue Switch", {"ON", "OFF"});
        configSwitch(CUE3_PARAM, 0.0f, 1.f, 1.0f, "Channel 3 Cue Switch", {"ON", "OFF"});
        configSwitch(CUE4_PARAM, 0.0f, 1.f, 1.0f, "Channel 4 Cue Switch", {"ON", "OFF"});

        configSwitch(PRE_X_PARAM, 0.0f, 1.f, 0.0f, "Aux X PRE / POST", {"POST", "PRE"});
        configSwitch(PRE_Y_PARAM, 0.0f, 1.f, 0.0f, "Aux Y PRE / POST", {"POST", "PRE"});

        configInput(L1_INPUT, "L1");
        configInput(L2_INPUT, "L2");
        configInput(L3_INPUT, "L3");
        configInput(L4_INPUT, "L4");
        configInput(R3_INPUT, "R3");
        configInput(R4_INPUT, "R4");

        configInput(AUX_X_L_INPUT, "Aux X L");
        configInput(AUX_X_R_INPUT, "Aux X R");

        configInput(AUX_Y_L_INPUT, "Aux Y L");
        configInput(AUX_Y_R_INPUT, "Aux Y R");

        configInput(CV_VOL_1, "CV Vol 1");
        configInput(CV_PAN_1, "CV Pan 1");
        configInput(CV_AUX_1, "CV Aux 1");

        configInput(CV_VOL_2, "CV Vol 2");
        configInput(CV_PAN_2, "CV Pan 2");
        configInput(CV_AUX_2, "CV Aux 2");

        configInput(CV_VOL_3, "CV Vol 3");
        configInput(CV_PAN_3, "CV Pan 3");

        configInput(CV_VOL_4, "CV Vol 4");
        configInput(CV_PAN_4, "CV Pan 4");

        configOutput(L_OUTPUT, "L");
        configOutput(R_OUTPUT, "R");

        configOutput(CUE_OUTPUT, "Cue");
        configOutput(PHONES_OUTPUT, "Phones");

        configOutput(X_AUX_OUTPUT, "X Aux");
        configOutput(Y_AUX_OUTPUT, "Y Aux"); 
    }

    float slew(float in, float out, float delta) {
        float error = (in)-out;
        float d = (delta);
        if (error > d) {
            error = d;
        } else if (error < -d) {
            error = -d;
        }
        out += error;
        return out;
    }

    void process(const ProcessArgs &args) override
    {
        // Read voltage inputs
        for(uint32_t i = 0; i < MIXER_CHANNELS; i++){
            l_input[i] = inputs[i].getVoltage();
            // Check if we have to normalize L to R on Stereo Inputs
            if(inputs[R1_INPUT + i].isConnected()){
                r_input[i] = inputs[R1_INPUT + i].getVoltage();
            } else {
                r_input[i] = l_input[i];
            }
            mono_in[i] = (l_input[i] + r_input[i]) / 2.0f;
        }

        // Do CV signals normalization if required and save the CV values
        for(uint32_t i = 0; i < MIXER_CHANNELS; i++){
            if(inputs[CV_VOL_1 + i].isConnected()){
                cv_vol[i] = inputs[CV_VOL_1 + i].getVoltage();
            } else {
                cv_vol[i] = 5.0f;
            }
            cv_pan[i] = inputs[CV_PAN_1 + i].getVoltage();
        }

        // Read voltage inputs
        for(uint32_t i = 0; i < MIXER_AUX; i++){
            // Read L channel
            aux_ret_l[i] = inputs[AUX_X_L_INPUT + i].getVoltage();
            // Check if we have to normalize L to R on AUX returns
            if(inputs[AUX_X_R_INPUT + i].isConnected()){
                aux_ret_r[i] = inputs[AUX_X_R_INPUT + i].getVoltage();
            } else {
                aux_ret_r[i] = aux_ret_l[i];
            }
        }

        cv_aux[0] = inputs[CV_AUX_1].getVoltage();
        cv_aux[1] = inputs[CV_AUX_2].getVoltage();

        for(uint32_t i = 0; i < MIXER_AUX; i++){
            prePost[i] = !params[PRE_X_PARAM + i].getValue();
            gain_ret[i] = params[AUX_X_VOL_PARAM + i].getValue();
            mix_ret_l += aux_ret_l[i] * gain_ret[i];
            mix_ret_r += aux_ret_r[i] * gain_ret[i];
        }

        // Check MUTE & CUE state on buttons
        for(uint32_t i = 0; i < MIXER_CHANNELS; i++){
            // Read the actual CUE button state
            isCued[i] = (bool)!(params[(CUE1_PARAM + i)].getValue() > 0.5f);
            // Read the actual MUTE button state
            isPressed[i] = (bool)(params[(MUTE1_PARAM + i)].getValue() > 0.5f);
            // Check for transition from pressed to not pressed
            if(!isPressed[i] && wasPressed[i]){
                // Toggle mute state only when mouse is released
                isMuted[i] = !isMuted[i];
            } 
            // Update the state for the next process call
            wasPressed[i] = isPressed[i];
        }
        for(uint32_t i = 0; i < MIXER_CHANNELS; i++){
            // Compute final mute value
            isFinallyMuted[i] = !isMuted[i] & !isGateMuted[i];

            // Write the MUTE LEDs state
            lights[MUTE1_LIGHT + i].setSmoothBrightness(!isFinallyMuted[i], LED_SMOOTHING);

            // Add slew to the MUTE and CUE params
            slewMute[i] = slew((float)isFinallyMuted[i], slewMute[i], SLEW_SMOOTHING);
            slewCue[i] = slew((float)isCued[i], slewCue[i], SLEW_SMOOTHING);
        }

        // Get channel main parameters
        for(uint32_t i = 0; i < MIXER_CHANNELS; i++){
            gain_pre[i] = clamp((params[VOL1_PARAM + i].getValue() * (cv_vol[i] / 5.0f)) * slewMute[i], 0.0f, 1.0f);
            pan_pre[i] = clamp((params[PAN1_PARAM + i].getValue() + (cv_pan[i] / 5.0f)), 0.0f, 1.0f);
            aux_pre[i] = clamp((params[AUX1_PARAM + i].getValue() + (cv_aux[i] / 5.0f)), -1.0f, 1.0f);
        }

        // Compute channel send
        for (uint32_t i = 0; i < MIXER_CHANNELS; i++){
            // Determine the routing for the auxiliary signal based on its voltage.
            if(aux_pre[i] >= 0.0f) sendtoX[i] = false; else sendtoX[i] = true;
            if(prePost[0] == 0) x_send += (mono_in[i] * fabsf(aux_pre[i]) * static_cast<float>(sendtoX[i]));  else x_send += (mono_in[i] * fabsf(aux_pre[i]) * gain_pre[i] * static_cast<float>(sendtoX[i]));
            if(prePost[1] == 0) y_send += (mono_in[i] * fabsf(aux_pre[i]) * static_cast<float>(!sendtoX[i])); else y_send += (mono_in[i] * fabsf(aux_pre[i]) * gain_pre[i] * static_cast<float>(!sendtoX[i]));
        }

        // Write the AUX output voltages
        outputs[X_AUX_OUTPUT].setVoltage(x_send);
        outputs[Y_AUX_OUTPUT].setVoltage(y_send);

        // Compute channel master & cue mix
        for (uint32_t i = 0; i < MIXER_CHANNELS; i++){
            l_output[i] = (l_input[i] * gain_pre[i]) * cosf(pan_pre[i] * M_PI_2);
            r_output[i] = (r_input[i] * gain_pre[i]) * sinf(pan_pre[i] * M_PI_2);
            mix_l += l_output[i];
            mix_r += r_output[i];
            mix_cue += mono_in[i] * slewCue[i];
        }

        // Sum returns to the master channel mix
        mix_l += mix_ret_l;
        mix_r += mix_ret_r;

        // Read master volume knob
        float masterVol = params[MASTER_VOL_PARAM].getValue();

        float phonesVol = params[PHONES_VOL_PARAM].getValue();
        float phonesMix = params[PHONES_MIX_PARAM].getValue();

        // Write the output voltages
        outputs[L_OUTPUT].setVoltage(mix_l * masterVol);
        outputs[R_OUTPUT].setVoltage(mix_r * masterVol);

        outputs[CUE_OUTPUT].setVoltage(mix_cue);
        // Set the number of channels for the STEREO_OUTPUT to 2 (for stereo)
        outputs[PHONES_OUTPUT].setChannels(2);
        outputs[PHONES_OUTPUT].setVoltage(((1.0f - phonesMix) * mix_cue) + (mix_l * phonesMix) * phonesVol, 0); 
        outputs[PHONES_OUTPUT].setVoltage(((1.0f - phonesMix) * mix_cue) + (mix_r * phonesMix) * phonesVol, 1); 

        // Set the fader LEDs brightness depending on channel signal
        for(uint32_t i = 0; i < MIXER_CHANNELS; i++){
            lights[VOL1_LIGHT + i].setSmoothBrightness(mono_in[i] / 1.0, LED_SMOOTHING);
        }

        // Set the LEDs brightness depending on output signal
        lights[L_LIGHT].setSmoothBrightness(mix_l / 5.0, LED_SMOOTHING);
        lights[R_LIGHT].setSmoothBrightness(mix_r / 5.0, LED_SMOOTHING);

        // Use a comparator for the clip leds
        if(abs(mix_l) > 10.0f){
            lights[CLIPL_LIGHT].setSmoothBrightness(1.0f, LED_SMOOTHING);
        } else {
            lights[CLIPL_LIGHT].setSmoothBrightness(0.0f, LED_SMOOTHING);
        }

        if(abs(mix_r) > 10.0f){
            lights[CLIPR_LIGHT].setSmoothBrightness(1.0f, LED_SMOOTHING);
        } else {
            lights[CLIPR_LIGHT].setSmoothBrightness(0.0f, LED_SMOOTHING);
        }

        // Check if an expander is connected
        bool expanderConnected = leftExpander.module && (leftExpander.module->model == modelEXP4);

        if (expanderConnected) {
            // Example: use data from the expander

                // Read or write to sharedData as needed
                for(uint32_t i = 0; i < MIXER_CHANNELS; i++){
                    // Read the shared data
                    sharedData.shared_l_output[i] = l_output[i];
                    sharedData.shared_r_output[i] = r_output[i];
                    isGateMuted[i] = sharedData.shared_gateMuted[i];
                }

                cv_aux[2] = sharedData.shared_cv_aux[2];
                cv_aux[3] = sharedData.shared_cv_aux[3];

                // Set the shared data for the expander to read
                leftExpander.producerMessage = &sharedData;
        }

    // Rest of your processing code

        x_send = 0.0f;
        y_send = 0.0f;

        mix_l = 0.0f;
        mix_r = 0.0f;

        mix_ret_l = 0.0f;
        mix_ret_r = 0.0f;
        mix_cue = 0.0f;
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();

        // Create a JSON array to store isFinallyMuted values
        json_t* mutedArray = json_array();
        for (int i = 0; i < MIXER_CHANNELS; ++i) {
            // Convert each element of isFinallyMuted to JSON
            json_t* mutedValue = json_boolean(isMuted[i]);
            json_array_append_new(mutedArray, mutedValue);
        }

        // Add the mutedArray to the root object
        json_object_set_new(rootJ, "isFinallyMuted", mutedArray);

        return rootJ;
}

    void dataFromJson(json_t* rootJ) override {
        // Extract the array of isFinallyMuted values from the JSON object
        json_t* mutedArray = json_object_get(rootJ, "isFinallyMuted");
        if (mutedArray) {
            for (int i = 0; i < MIXER_CHANNELS; ++i) {
                json_t* mutedValue = json_array_get(mutedArray, i);
                if (mutedValue)
                    isMuted[i] = json_boolean_value(mutedValue);
            }
        }
}

};

struct PerformanceMixerWidget : ModuleWidget
{
    PerformanceMixerWidget(PerformanceMixer *module)
    {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PerformanceMixer.svg")));

		addChild(createWidget<ScrewSilver>(Vec(14, 1.5)));
        addChild(createWidget<ScrewSilver>(Vec(14, 363.5)));
        addChild(createWidget<ScrewSilver>(Vec(392, 1.5)));
		addChild(createWidget<ScrewSilver>(Vec(392, 363.5)));

        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(11.35, 44.5)), module, PerformanceMixer::AUX1_PARAM));
        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(33.85, 44.5)), module, PerformanceMixer::AUX2_PARAM));
        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(56.35, 44.5)), module, PerformanceMixer::AUX3_PARAM));
        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(78.85, 44.5)), module, PerformanceMixer::AUX4_PARAM));

        addParam(createParamCentered<Trimpot>(mm2px(Vec(11.35, 65)), module, PerformanceMixer::PAN1_PARAM));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(33.85, 65)), module, PerformanceMixer::PAN2_PARAM));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(56.35, 65)), module, PerformanceMixer::PAN3_PARAM));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(78.85, 65)), module, PerformanceMixer::PAN4_PARAM));

        addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(128.75, 44.5)), module, PerformanceMixer::PHONES_VOL_PARAM));
        addParam(createParamCentered<Davies1900hBlackKnob>(mm2px(Vec(128.75, 65.0)), module, PerformanceMixer::PHONES_MIX_PARAM));
        addParam(createParamCentered<Davies1900hBlackKnob>(mm2px(Vec(102.5, 85.5)), module, PerformanceMixer::AUX_X_VOL_PARAM)); 
        addParam(createParamCentered<Davies1900hBlackKnob>(mm2px(Vec(102.5, 106.0)), module, PerformanceMixer::AUX_Y_VOL_PARAM));   
        addParam(createParamCentered<Davies1900hLargeBlackKnob>(mm2px(Vec(129.0, 102.0)), module, PerformanceMixer::MASTER_VOL_PARAM));       

        addParam(createLightParamCentered<LEDSliderRed>(mm2px(Vec(16.25, 95.5)), module, PerformanceMixer::VOL1_PARAM, PerformanceMixer::VOL1_LIGHT));
        addParam(createLightParamCentered<LEDSliderRed>(mm2px(Vec(38.75, 95.5)), module, PerformanceMixer::VOL2_PARAM, PerformanceMixer::VOL2_LIGHT));
        addParam(createLightParamCentered<LEDSliderRed>(mm2px(Vec(61.25, 95.5)), module, PerformanceMixer::VOL3_PARAM, PerformanceMixer::VOL3_LIGHT));
        addParam(createLightParamCentered<LEDSliderRed>(mm2px(Vec(83.25, 95.5)), module, PerformanceMixer::VOL4_PARAM, PerformanceMixer::VOL4_LIGHT));

        addParam(createParamCentered<NANOComponents::BarkSwitchSmall2P>(mm2px(Vec(6.25, 95.5)), module, PerformanceMixer::CUE1_PARAM));
        addParam(createParamCentered<NANOComponents::BarkSwitchSmall2P>(mm2px(Vec(28.75, 95.5)), module, PerformanceMixer::CUE2_PARAM));
        addParam(createParamCentered<NANOComponents::BarkSwitchSmall2P>(mm2px(Vec(51.25, 95.5)), module, PerformanceMixer::CUE3_PARAM));
        addParam(createParamCentered<NANOComponents::BarkSwitchSmall2P>(mm2px(Vec(73.75, 95.5)), module, PerformanceMixer::CUE4_PARAM));

        addParam(createParamCentered<NANOComponents::BarkSwitchSmall2P>(mm2px(Vec(95.5, 65)), module, PerformanceMixer::PRE_X_PARAM));
        addParam(createParamCentered<NANOComponents::BarkSwitchSmall2P>(mm2px(Vec(109.5, 65)), module, PerformanceMixer::PRE_Y_PARAM));

        addParam(createParamCentered<LEDButton>(mm2px(Vec(6.25, 109.5)), module, PerformanceMixer::MUTE1_PARAM));
        addChild(createLightCentered<LargeLight<RedLight>>(mm2px(Vec(6.25, 109.5)), module, PerformanceMixer::MUTE1_LIGHT));

        addParam(createParamCentered<LEDButton>(mm2px(Vec(28.75, 109.5)), module, PerformanceMixer::MUTE2_PARAM));
        addChild(createLightCentered<LargeLight<RedLight>>(mm2px(Vec(28.75, 109.5)), module, PerformanceMixer::MUTE2_LIGHT));

        addParam(createParamCentered<LEDButton>(mm2px(Vec(51.25, 109.5)), module, PerformanceMixer::MUTE3_PARAM));
        addChild(createLightCentered<LargeLight<RedLight>>(mm2px(Vec(51.25, 109.5)), module, PerformanceMixer::MUTE3_LIGHT));

        addParam(createParamCentered<LEDButton>(mm2px(Vec(73.75, 109.5)), module, PerformanceMixer::MUTE4_PARAM));
        addChild(createLightCentered<LargeLight<RedLight>>(mm2px(Vec(73.75, 109.5)), module, PerformanceMixer::MUTE4_LIGHT));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.25,  14.50)), module, PerformanceMixer::L1_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(28.75, 14.50)), module, PerformanceMixer::L2_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(51.25, 14.50)), module, PerformanceMixer::L3_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(73.75, 14.50)), module, PerformanceMixer::L4_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(61.25, 14.50)), module, PerformanceMixer::R3_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(83.75, 14.50)), module, PerformanceMixer::R4_INPUT));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(97.50, 27.50)), module, PerformanceMixer::AUX_X_L_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(107.50, 27.50)), module, PerformanceMixer::AUX_X_R_INPUT));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(97.50, 40.50)), module, PerformanceMixer::AUX_Y_L_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(107.50, 40.50)), module, PerformanceMixer::AUX_Y_R_INPUT));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.25, 27.50)), module, PerformanceMixer::CV_VOL_1));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(16.25, 27.50)), module, PerformanceMixer::CV_PAN_1));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(16.25, 14.50)), module, PerformanceMixer::CV_AUX_1));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(28.75, 27.50)), module, PerformanceMixer::CV_VOL_2));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(38.75, 27.50)), module, PerformanceMixer::CV_PAN_2));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(38.75, 14.50)), module, PerformanceMixer::CV_AUX_2));
    
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(51.25, 27.50)), module, PerformanceMixer::CV_VOL_3));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(61.25, 27.50)), module, PerformanceMixer::CV_PAN_3));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(73.75, 27.50)), module, PerformanceMixer::CV_VOL_4));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(83.75, 27.50)), module, PerformanceMixer::CV_PAN_4));        

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(123.75, 14.50)), module, PerformanceMixer::L_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(133.75, 14.50)), module, PerformanceMixer::R_OUTPUT));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(123.75, 27.50)), module, PerformanceMixer::CUE_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(133.75, 27.50)), module, PerformanceMixer::PHONES_OUTPUT));        

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(97.50, 14.50)), module, PerformanceMixer::X_AUX_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(107.50, 14.50)), module, PerformanceMixer::Y_AUX_OUTPUT));        

        addChild(createLightCentered<MediumLight<WhiteLight>>(mm2px(Vec(122.75, 85.25)), module, PerformanceMixer::L_LIGHT));
        addChild(createLightCentered<MediumLight<WhiteLight>>(mm2px(Vec(134.75, 85.25)), module, PerformanceMixer::R_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(122.75, 80.5)), module, PerformanceMixer::CLIPL_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(134.75, 80.5)), module, PerformanceMixer::CLIPR_LIGHT));
    }
};

Model *modelPerformanceMixer = createModel<PerformanceMixer, PerformanceMixerWidget>("PerformanceMixer");