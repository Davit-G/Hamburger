#pragma once
 
#include "../Panel.h"
#include "../../Knob.h"

class TapeSatPanel : public Panel
{
public:
    TapeSatPanel(AudioPluginAudioProcessor &p) : Panel(p, "TAPE"), 
        drive(p, "DRIVE", ParamIDs::tapeDrive),
        saturation(p, "SAT", ParamIDs::tapeSaturation),
        tapeWidth(p, "WIDTH", ParamIDs::tapeWidth)
    {
        addAndMakeVisible(drive);
        addAndMakeVisible(saturation);
        addAndMakeVisible(tapeWidth);

        Palette::setKnobColoursOfComponent(&drive, Palette::colours[4]);
        Palette::setKnobColoursOfComponent(&saturation, Palette::colours[4]);
        Palette::setKnobColoursOfComponent(&tapeWidth, Palette::colours[4]);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        saturation.setBounds(bounds.removeFromTop(static_cast<int>(bounds.getHeight() * 0.6666f)).reduced(10));

        auto width = bounds.getWidth() / 2;
        tapeWidth.setBounds(bounds.removeFromLeft(width));
        drive.setBounds(bounds);
    }

    ParamKnob drive;
    ParamKnob tapeWidth;
    ParamKnob saturation;
};