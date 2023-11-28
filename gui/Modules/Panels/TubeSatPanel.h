#pragma once

 
#include "../Panel.h"
#include "../../Knob.h"
#include "PanelNames.h"

class TubeSatPanel : public Panel
{
public:
    TubeSatPanel(AudioPluginAudioProcessor &p) : Panel(p, "TUBE"), 
        tubeTone(p, "TUBE TONE", "tubeTone"),
        drive(p, "DRIVE", "saturationAmount", ParamUnits::percent)
    {
        addAndMakeVisible(tubeTone);
        addAndMakeVisible(drive);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        drive.setBounds(bounds.removeFromTop(bounds.getHeight() / 1.5f).reduced(10));

        auto width = bounds.getWidth() / 1;
        tubeTone.setBounds(bounds.removeFromLeft(width));
    }

    ParamKnob tubeTone;
    ParamKnob drive;
};