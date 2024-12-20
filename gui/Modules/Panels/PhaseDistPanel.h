#pragma once
 
#include "../Panel.h"
#include "../../Knob.h"



class PhaseDistPanel : public Panel
{
public:
    PhaseDistPanel(AudioPluginAudioProcessor &p) : Panel(p, "PHASE", Palette::colours[3]), 
        satKnob(p, "SATURATION", ParamIDs::phaseAmount, ParamUnits::percent),
        toneKnob(p, "TONE", ParamIDs::phaseDistTone, ParamUnits::hz),
        normKnob(p, "STEREO", ParamIDs::phaseDistStereo),
        rectKnob(p, "RECTIFY", ParamIDs::phaseRectify),
        shiftKnob(p, "SHIFT", ParamIDs::phaseShift)
    {
        addAndMakeVisible(satKnob);
        addAndMakeVisible(toneKnob);
        addAndMakeVisible(normKnob);
        addAndMakeVisible(shiftKnob);
        addAndMakeVisible(rectKnob);

        Palette::setKnobColoursOfComponent(&satKnob, Palette::colours[3]);
        Palette::setKnobColoursOfComponent(&toneKnob, Palette::colours[3]);
        Palette::setKnobColoursOfComponent(&normKnob, Palette::colours[3]);
        Palette::setKnobColoursOfComponent(&rectKnob, Palette::colours[3]);
        Palette::setKnobColoursOfComponent(&shiftKnob, Palette::colours[3]);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        satKnob.setBounds(bounds.removeFromTop(bounds.getHeight() / 1.5f).reduced(10));

        auto width = bounds.getWidth() / 4;
        toneKnob.setBounds(bounds.removeFromLeft(width));
        rectKnob.setBounds(bounds.removeFromLeft(width));
        shiftKnob.setBounds(bounds.removeFromLeft(width));
        normKnob.setBounds(bounds);
    }

private:
    ParamKnob satKnob;
    ParamKnob toneKnob;
    ParamKnob normKnob;
    ParamKnob rectKnob;
    ParamKnob shiftKnob;
};