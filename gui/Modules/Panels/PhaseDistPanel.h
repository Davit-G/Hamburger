#pragma once
 
#include "../Panel.h"
#include "../../Knob.h"



class PhaseDistPanel : public Panel
{
public:
    PhaseDistPanel(AudioPluginAudioProcessor &p) : Panel(p, "PHASE"), 
        satKnob(p, "SATURATION", ParamIDs::phaseAmount, ParamUnits::percent),
        toneKnob(p, "TONE", ParamIDs::phaseDistTone, ParamUnits::hz),
        normKnob(p, "STEREO", ParamIDs::phaseDistStereo),
        rectKnob(p, "RECTIFY", ParamIDs::phaseRectify)
    {
        addAndMakeVisible(satKnob);
        addAndMakeVisible(toneKnob);
        addAndMakeVisible(normKnob);
        addAndMakeVisible(rectKnob);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        satKnob.setBounds(bounds.removeFromTop(bounds.getHeight() / 1.5f).reduced(10));

        auto width = bounds.getWidth() / 3;
        toneKnob.setBounds(bounds.removeFromLeft(width));
        rectKnob.setBounds(bounds.removeFromLeft(width));
        normKnob.setBounds(bounds);
    }

private:
    ParamKnob satKnob;
    ParamKnob toneKnob;
    ParamKnob normKnob;
    ParamKnob rectKnob;
};