#pragma once
 
#include "../Panel.h"
#include "../../Knob.h"



class PhaseDistPanel : public Panel
{
public:
    PhaseDistPanel(AudioPluginAudioProcessor &p) : Panel(p, "PHASE"), 
        satKnob(p, "SATURATION", ParamIDs::phaseAmount, ParamUnits::percent),
        toneKnob(p, "TONE", ParamIDs::phaseDistTone, ParamUnits::hz),
        normKnob(p, "NORMALIZE", ParamIDs::phaseDistNormalise)
    {
        addAndMakeVisible(satKnob);
        addAndMakeVisible(toneKnob);
        addAndMakeVisible(normKnob);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        satKnob.setBounds(bounds.removeFromTop(bounds.getHeight() / 1.5f).reduced(10));

        auto width = bounds.getWidth() / 2;
        toneKnob.setBounds(bounds.removeFromLeft(width));
        normKnob.setBounds(bounds);
    }

private:
    ParamKnob satKnob;
    ParamKnob toneKnob;
    ParamKnob normKnob;
};