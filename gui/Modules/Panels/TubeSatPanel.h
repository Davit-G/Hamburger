#pragma once

 
#include "../Panel.h"
#include "../../Knob.h"




class TubeSatPanel : public Panel
{
public:
    TubeSatPanel(AudioPluginAudioProcessor &p) : Panel(p, "TUBE"), 
        tubeTone(p, "TUBE TONE", ParamIDs::tubeTone),
        bias(p, "BIAS", ParamIDs::tubeBias),
        jeff(p, "JEFF", ParamIDs::jeffAmount),
        drive(p, "DRIVE", ParamIDs::tubeAmount, ParamUnits::percent)
    {
        addAndMakeVisible(tubeTone);
        addAndMakeVisible(drive);
        addAndMakeVisible(bias);
        addAndMakeVisible(jeff);

        Palette::setKnobColoursOfComponent(&tubeTone, Palette::colours[4]);
        Palette::setKnobColoursOfComponent(&drive, Palette::colours[4]);
        Palette::setKnobColoursOfComponent(&bias, Palette::colours[4]);
        Palette::setKnobColoursOfComponent(&jeff, Palette::colours[4]);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        drive.setBounds(bounds.removeFromTop(static_cast<int>(bounds.getHeight() * 0.6666f)).reduced(10));

        auto width = bounds.getWidth() / 3;
        tubeTone.setBounds(bounds.removeFromLeft(width));
        jeff.setBounds(bounds.removeFromLeft(width));
        bias.setBounds(bounds);
    }

    ParamKnob tubeTone;
    ParamKnob drive;
    ParamKnob jeff;
    ParamKnob bias;
};