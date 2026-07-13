#pragma once

 
#include "../Panel.h"
#include "../../Knob.h"




class SlewRatePanel : public Panel
{
public:
    SlewRatePanel(AudioPluginAudioProcessor &p) : Panel(p, "SLEW"), 
        ema(p, "EMA", ParamIDs::emaParam),
        alpha(p, "ALPHA", ParamIDs::alphaParam),
        bias(p, "TONE", ParamIDs::slewSpeed),
        directionality(p, "BEND", ParamIDs::directionality),
        type(p, "TYPE", ParamIDs::slewType)
    {
        addAndMakeVisible(ema);
        addAndMakeVisible(bias);
        addAndMakeVisible(alpha);
        addAndMakeVisible(directionality);
        addAndMakeVisible(type);

        Palette::setKnobColoursOfComponent(&ema, Palette::colours[4]);
        Palette::setKnobColoursOfComponent(&alpha, Palette::colours[4]);
        Palette::setKnobColoursOfComponent(&bias, Palette::colours[4]);
        Palette::setKnobColoursOfComponent(&type, Palette::colours[4]);
        Palette::setKnobColoursOfComponent(&directionality, Palette::colours[4]);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        alpha.setBounds(bounds.removeFromTop(static_cast<int>(bounds.getHeight() * 0.6666f)).reduced(10));

        auto width = bounds.getWidth() / 4;
        bias.setBounds(bounds.removeFromLeft(width));
        directionality.setBounds(bounds.removeFromLeft(width));
        ema.setBounds(bounds.removeFromLeft(width));
        type.setBounds(bounds);
    }

    ParamKnob ema;
    ParamKnob alpha;
    ParamKnob bias;
    ParamKnob directionality;
    ParamKnob type;
};