#include "plugin.hpp"

struct MAR : Module
{

	float xValue = 0.0f, yValue = 0.0f;

	enum ParamIds
	{
		Y_POT_PARAM,
		X2_POT_PARAM,
		X1_POT_PARAM,
		X4_POT_PARAM,
		X3_POT_PARAM,
		NUM_PARAMS
	};
	enum InputIds
	{
		Y2_IN_INPUT,
		Y1_IN_INPUT,
		X1_IN_INPUT,
		X2_IN_INPUT,
		Y4_IN_INPUT,
		Y3_IN_INPUT,
		X4_IN_INPUT,
		X3_IN_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
		XINV_OUT_OUTPUT,
		XPLUSY_OUT_OUTPUT,
		Y_OUT_OUTPUT,
		X_OUT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds
	{
		ENUMS(X_LIGHT, 3),
		ENUMS(Y_LIGHT, 3),
		NUM_LIGHTS
	};

	MAR()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(Y_POT_PARAM, 0.f, 1.f, 0.f, "Mixer Y gain");
		configParam(X2_POT_PARAM, 0.f, 1.f, 0.f, "Input X2 gain");
		configParam(X1_POT_PARAM, 0.f, 1.f, 0.f, "Input X1 gain");
		configParam(X4_POT_PARAM, 0.f, 1.f, 0.f, "Input X4 gain");
		configParam(X3_POT_PARAM, 0.f, 1.f, 0.f, "Input X3 gain");

		configInput(X1_IN_INPUT, "X1");
		configInput(X2_IN_INPUT, "X2");
		configInput(X3_IN_INPUT, "X3");
		configInput(X4_IN_INPUT, "X4");

		configInput(Y1_IN_INPUT, "Y1");
		configInput(Y2_IN_INPUT, "Y2");
		configInput(Y3_IN_INPUT, "Y3");
		configInput(Y4_IN_INPUT, "Y4");

		configOutput(X_OUT_OUTPUT, "X");
		configOutput(XINV_OUT_OUTPUT, "X INV");
		configOutput(Y_OUT_OUTPUT, "Y");
		configOutput(XPLUSY_OUT_OUTPUT, "X+Y");
	}

	void process(const ProcessArgs &args) override
	{

		if (inputs[Y1_IN_INPUT].isConnected())
		{

			xValue =
				inputs[X1_IN_INPUT].getVoltage() * params[X1_POT_PARAM].getValue() +
				inputs[X2_IN_INPUT].getVoltage() * params[X2_POT_PARAM].getValue() +
				inputs[X3_IN_INPUT].getVoltage() * params[X3_POT_PARAM].getValue() +
				inputs[X4_IN_INPUT].getVoltage() * params[X4_POT_PARAM].getValue();

			yValue =
				(inputs[Y1_IN_INPUT].getVoltage() +
				 inputs[Y2_IN_INPUT].getVoltage() +
				 inputs[Y3_IN_INPUT].getVoltage() +
				 inputs[Y4_IN_INPUT].getVoltage()) *
				params[Y_POT_PARAM].getValue();
		}
		else
		{

			xValue =
				inputs[X1_IN_INPUT].getVoltage() * params[X1_POT_PARAM].getValue() +
				inputs[X2_IN_INPUT].getVoltage() * params[X2_POT_PARAM].getValue() +
				inputs[X3_IN_INPUT].getVoltage() * params[X3_POT_PARAM].getValue() +
				inputs[X4_IN_INPUT].getVoltage() * params[X4_POT_PARAM].getValue();

			yValue =
				(xValue +
				 inputs[Y2_IN_INPUT].getVoltage() +
				 inputs[Y3_IN_INPUT].getVoltage() +
				 inputs[Y4_IN_INPUT].getVoltage()) *
				params[Y_POT_PARAM].getValue();
		}

		outputs[X_OUT_OUTPUT].setVoltage(xValue);
		outputs[XINV_OUT_OUTPUT].setVoltage(-xValue);
		outputs[Y_OUT_OUTPUT].setVoltage(yValue);
		outputs[XPLUSY_OUT_OUTPUT].setVoltage(xValue + yValue);

		lights[X_LIGHT + 0].setSmoothBrightness(fmaxf(0.0, xValue / 5.0), 0.0001f);
		lights[X_LIGHT + 0].setSmoothBrightness(fmaxf(0.0, -xValue / 5.0), 0.0001f);
		lights[X_LIGHT + 1].setSmoothBrightness(fmaxf(0.0, -xValue / 5.0), 0.0001f);

		lights[Y_LIGHT + 0].setSmoothBrightness(fmaxf(0.0, yValue / 5.0), 0.0001f);
		lights[Y_LIGHT + 0].setSmoothBrightness(fmaxf(0.0, -yValue / 5.0), 0.0001f);
		lights[Y_LIGHT + 1].setSmoothBrightness(fmaxf(0.0, -yValue / 5.0), 0.0001f);
	}
};

struct MARWidget : ModuleWidget
{
	MARWidget(MAR *module)
	{
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/MAR.svg")));

		addChild(createWidget<ScrewSilver>(Vec(14, 1.5)));
		//addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		//addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(90, 363.5)));

		addParam(createParamCentered<Davies1900hLargeBlackKnob>(mm2px(Vec(20, 18.25)), module, MAR::Y_POT_PARAM));
		addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(9, 43.35)), module, MAR::X1_POT_PARAM));
		addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(31, 43.35)), module, MAR::X2_POT_PARAM));
		addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(9, 66.35)), module, MAR::X3_POT_PARAM));
		addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(31, 66.35)), module, MAR::X4_POT_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5, 82.25)), module, MAR::X1_IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15, 82.25)), module, MAR::X2_IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5, 95.25)), module, MAR::X3_IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15, 95.25)), module, MAR::X4_IN_INPUT));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25, 82.25)), module, MAR::Y1_IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(35, 82.25)), module, MAR::Y2_IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25, 95.25)), module, MAR::Y3_IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(35, 95.25)), module, MAR::Y4_IN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5, 108.25)), module, MAR::X_OUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15, 108.25)), module, MAR::XINV_OUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(25, 108.25)), module, MAR::Y_OUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(35, 108.25)), module, MAR::XPLUSY_OUT_OUTPUT));

		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(20, 34.25)), module, MAR::Y_LIGHT));
		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(20, 54.75)), module, MAR::X_LIGHT));
	}
};

Model *modelMAR = createModel<MAR, MARWidget>("MAR");