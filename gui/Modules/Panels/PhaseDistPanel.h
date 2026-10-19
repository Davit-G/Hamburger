#pragma once

#include "../Panel.h"
#include "../../SaturationIcons/CentredSVGIcon.h"

class PhaseDistPanel : public Panel
{
public:
    PhaseDistPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::main, 0}) : Panel(p, "PHASE", Palette::colours[3]),
        satKnob(p, "SATURATION", slot, ParamIDs::phaseAmount, ValueType::percent, ScopeContextType::IN_OUT),
        toneKnob(p, "TONE", slot, ParamIDs::phaseDistTone, ValueType::hz, ScopeContextType::IN_OUT),
        normKnob(p, "STEREO", slot, ParamIDs::phaseDistStereo, ValueType::none, ScopeContextType::IN_OUT),
        rectKnob(p, "RECTIFY", slot, ParamIDs::phaseRectify, ValueType::none, ScopeContextType::IN_OUT),
        shiftKnob(p, "SHIFT", slot, ParamIDs::phaseShift, ValueType::none, ScopeContextType::IN_OUT),
        wave(BinaryData::Waves_svg, BinaryData::Waves_svgSize)
    {
        addKnobs({&satKnob, &toneKnob, &normKnob, &shiftKnob, &rectKnob});
        addAndMakeVisible(wave);
    }

    void resized() override { layoutSaturation(satKnob, wave, {&rectKnob, &normKnob, &shiftKnob, &toneKnob}); }

private:
    ParamKnob satKnob;
    RectSlider toneKnob, normKnob, rectKnob, shiftKnob;
    CentredSVGIcon wave;
};
