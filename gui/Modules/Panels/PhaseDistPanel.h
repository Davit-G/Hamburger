#pragma once
 
#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"
#include "../../Knob.h"



class PhaseDistPanel : public Panel
{
public:
    PhaseDistPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::main, 0}) : Panel(p, "PHASE", Palette::colours[3], slot), 
        satKnob(p, "SATURATION", slot, ParamIDs::phaseAmount, ScopeContextType::IN_OUT),
        toneKnob(p, "TONE", slot, ParamIDs::phaseDistTone, ScopeContextType::IN_OUT),
        normKnob(p, "STEREO", slot, ParamIDs::phaseDistStereo, ScopeContextType::IN_OUT),
        rectKnob(p, "RECTIFY", slot, ParamIDs::phaseRectify, ScopeContextType::IN_OUT),
        shiftKnob(p, "SHIFT", slot, ParamIDs::phaseShift, ScopeContextType::IN_OUT),
        wave(BinaryData::Waves_svg, BinaryData::Waves_svgSize)
    {
        addAndMakeVisible(satKnob);
        addAndMakeVisible(toneKnob);
        addAndMakeVisible(normKnob);
        addAndMakeVisible(shiftKnob);
        addAndMakeVisible(rectKnob);
        addAndMakeVisible(wave);
        
        const auto colour = Palette::colours[3];
        Palette::setKnobColoursOfComponent(this, colour);

        Palette::setKnobColoursOfComponent(&satKnob, colour);
        Palette::setKnobColoursOfComponent(&toneKnob, colour);
        Palette::setKnobColoursOfComponent(&normKnob, colour);
        Palette::setKnobColoursOfComponent(&rectKnob, colour);
        Palette::setKnobColoursOfComponent(&shiftKnob, colour);
    }

    void resized() override
    {
        if (usesCompactLayout())
            compactLayout(satKnob, wave, {&rectKnob, &normKnob, &shiftKnob, &toneKnob});
        else
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