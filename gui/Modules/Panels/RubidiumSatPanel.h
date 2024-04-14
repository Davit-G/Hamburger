#pragma once

 
#include "../Panel.h"
#include "../../Knob.h"
#include "PanelNames.h"

class RubidiumSatPanel : public Panel
{
public:
    RubidiumSatPanel(AudioPluginAudioProcessor &p) : Panel(p, "RUBIDIUM"), 
        tubeTone(p, "TONE", "rubidiumTone", ParamUnits::hz),
        mojo(p, "MOJO", "rubidiumMojo"),
        hysteresis(p, "ASYM", "rubidiumAsym"),
        drive(p, "DRIVE", "rubidiumAmount", ParamUnits::percent)
    {
        addAndMakeVisible(tubeTone);
        addAndMakeVisible(drive);
        addAndMakeVisible(mojo);
        addAndMakeVisible(hysteresis);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        drive.setBounds(bounds.removeFromTop(bounds.getHeight() / 1.5f).reduced(10));

        auto width = bounds.getWidth() / 3;
        tubeTone.setBounds(bounds.removeFromLeft(width));
        mojo.setBounds(bounds.removeFromLeft(width));
        hysteresis.setBounds(bounds);
    }

    ParamKnob tubeTone;
    ParamKnob drive;
    ParamKnob mojo;
    ParamKnob hysteresis;
};