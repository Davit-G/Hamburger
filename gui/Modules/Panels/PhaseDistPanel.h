#pragma once
 
#include "../Panel.h"
#include "../../Knob.h"



class PhaseDistPanel : public Panel
{
public:
    PhaseDistPanel(AudioPluginAudioProcessor &p) : Panel(p, "PHASE", Palette::colours[3]), 
        satKnob(p, "SATURATION", ParamIDs::phaseAmount, ParamUnits::percent, ScopeContextType::IN_OUT),
        toneKnob(p, "TONE", ParamIDs::phaseDistTone, ParamUnits::hz, ScopeContextType::IN_OUT),
        normKnob(p, "STEREO", ParamIDs::phaseDistStereo, ParamUnits::none, ScopeContextType::IN_OUT),
        rectKnob(p, "RECTIFY", ParamIDs::phaseRectify, ParamUnits::none, ScopeContextType::IN_OUT),
        shiftKnob(p, "SHIFT", ParamIDs::phaseShift, ParamUnits::none, ScopeContextType::IN_OUT),
        wave(BinaryData::Waves_svg, BinaryData::Waves_svgSize)
    {
        addAndMakeVisible(satKnob);
        addAndMakeVisible(toneKnob);
        addAndMakeVisible(normKnob);
        addAndMakeVisible(shiftKnob);
        addAndMakeVisible(rectKnob);
        addAndMakeVisible(wave);

        Palette::setKnobColoursOfComponent(&satKnob, Palette::colours[3]);
        Palette::setKnobColoursOfComponent(&toneKnob, Palette::colours[3]);
        Palette::setKnobColoursOfComponent(&normKnob, Palette::colours[3]);
        Palette::setKnobColoursOfComponent(&rectKnob, Palette::colours[3]);
        Palette::setKnobColoursOfComponent(&shiftKnob, Palette::colours[3]);
    }

    void resized() override
    {
        fiveKnobLayout(satKnob, wave, rectKnob, normKnob, shiftKnob, toneKnob);
    }

private:
    ParamKnob satKnob;
    RectSlider toneKnob;
    RectSlider normKnob;
    RectSlider rectKnob;
    RectSlider shiftKnob;

    CentredSVGIcon wave;
};