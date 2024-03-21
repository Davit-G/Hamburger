#pragma once

 
#include "../Panel.h"
#include "../../Knob.h"
#include "PanelNames.h"

class TubeSatPanel : public Panel
{
public:
    TubeSatPanel(AudioPluginAudioProcessor &p) : Panel(p, "TUBE"), 
        tubeTone(p, "TUBE TONE", "tubeTone"),
        bias(p, "BIAS", "bias"),
        jeff(p, "JEFF", "jeffAmount"),
        drive(p, "DRIVE", "tubeAmount", ParamUnits::percent)
    {
        addAndMakeVisible(tubeTone);
        addAndMakeVisible(drive);
        addAndMakeVisible(bias);
        addAndMakeVisible(jeff);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        drive.setBounds(bounds.removeFromTop(bounds.getHeight() / 1.5f).reduced(10));

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