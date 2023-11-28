#pragma once
 
#include "../Panel.h"
#include "../../Knob.h"
#include "PanelNames.h"

class PhaseDistPanel : public Panel
{
public:
    PhaseDistPanel(AudioPluginAudioProcessor &p) : Panel(p, "PHASE"), 
        satKnob(p, "SATURATION", "saturationAmount", ParamUnits::percent),
        toneKnob(p, "TONE", "phaseDistTone")
    {
        addAndMakeVisible(satKnob);
        addAndMakeVisible(toneKnob);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        satKnob.setBounds(bounds.removeFromTop(bounds.getHeight() / 1.5f).reduced(10));

        auto width = bounds.getWidth() / 4;
        toneKnob.setBounds(bounds.removeFromLeft(width));
    }

private:
    ParamKnob satKnob;
    ParamKnob toneKnob;
};