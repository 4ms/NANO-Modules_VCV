#pragma once
#include "componentlibrary.hpp"

namespace NANOComponents
{

    //Bark Switches used with the permission of Phil Golden the Author of Coirt / Bark plugins https://vcvrack.com/plugins#Coirt"

    struct BarkSwitchSmall2P : app::SvgSwitch
    {
        BarkSwitchSmall2P()
        {
            addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/BarkSwitchSmall_0.svg")));
            addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/BarkSwitchSmall_1.svg")));
        }
    };

    struct BarkSwitchSmallSide2P : app::SvgSwitch
    {
        BarkSwitchSmallSide2P()
        {
            addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/BarkSwitchSmallSide_0.svg")));
            addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/BarkSwitchSmallSide_2.svg")));
        }
    };

    struct BarkSwitchSmallSide3P : app::SvgSwitch
    {
        BarkSwitchSmallSide3P()
        {
            addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/BarkSwitchSmallSide_0.svg")));
            addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/BarkSwitchSmallSide_1.svg")));
            addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/BarkSwitchSmallSide_2.svg")));
        }
    };

    struct NANOSwithcKnob : app::SvgKnob {
	widget::SvgWidget* bg;

	NANOSwithcKnob() {
		bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);
        }
    };

    struct NANOBlackKnobSwitch : NANOSwithcKnob
    {
        NANOBlackKnobSwitch()
        {
            box.size = Vec(20, 20);
            minAngle = -0.6f * M_PI;
            maxAngle = 0.6f * M_PI;
            snap = true;
            
		setSvg(Svg::load(asset::system("res/ComponentLibrary/Davies1900hLargeBlack.svg")));
		bg->setSvg(Svg::load(asset::system("res/ComponentLibrary/Davies1900hLargeBlack_bg.svg")));
        }
    };
} // namespace NANOComponents
