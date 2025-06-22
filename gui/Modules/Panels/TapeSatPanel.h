#pragma once
 
#include "../Panel.h"
#include "../../Knob.h"

class TapeSatPanel : public Panel
{
public:
    TapeSatPanel(AudioPluginAudioProcessor &p) : Panel(p, "TAPE"), 
        drive(p, "DRIVE", ParamIDs::tapeDrive),
        bias(p, "BIAS", ParamIDs::tapeBias),
        tapeWidth(p, "AGE", ParamIDs::tapeWidth)
    {
        addAndMakeVisible(drive);
        addAndMakeVisible(bias);
        addAndMakeVisible(tapeWidth);

        Palette::setKnobColoursOfComponent(&drive, Palette::colours[4]);
        Palette::setKnobColoursOfComponent(&bias, Palette::colours[4]);
        Palette::setKnobColoursOfComponent(&tapeWidth, Palette::colours[4]);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        drive.setBounds(bounds.removeFromTop(static_cast<int>(bounds.getHeight() * 0.6666f)).reduced(10));

        auto width = bounds.getWidth() / 2;
        tapeWidth.setBounds(bounds.removeFromLeft(width));
        bias.setBounds(bounds);
    }

    ParamKnob drive;
    ParamKnob tapeWidth;
    ParamKnob bias;
};