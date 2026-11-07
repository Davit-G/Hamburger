#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"
#include "../../SaturationIcons/CentredSVGIcon.h"

class ClassicSatPanel : public Panel
{
public:
    ClassicSatPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::main, 0}) : Panel(p, "GRILL", &Theme::grill, slot),
        satKnob(p, "SATURATION", slot, ParamIDs::saturationAmount, ScopeContextType::IN_OUT),
        biasKnob(p, "DC BIAS", slot, ParamIDs::grillBias, ScopeContextType::IN_OUT),
        fuzzKnob(p, "DIODE", slot, ParamIDs::diode, ScopeContextType::IN_OUT),
        cookedKnob(p, "WAVEFOLD", slot, ParamIDs::fold, ScopeContextType::IN_OUT),
        dcTimingKnob(p, "DC SLEW", slot, ParamIDs::grillDcTiming, ScopeContextType::IN_OUT),
        tube(BinaryData::Grill_svg, BinaryData::Grill_svgSize, &Theme::grill, -3)
    {
        addKnobs({&satKnob, &biasKnob, &fuzzKnob, &cookedKnob, &dcTimingKnob});
        addAndMakeVisible(tube);
    }

    // the timing sits under the bias it times, wavefold and diode on the other side
    void resized() override { layoutSaturation(satKnob, tube, {&biasKnob, &dcTimingKnob, &cookedKnob, &fuzzKnob}); }

private:
    ParamKnob satKnob;
    RectSlider biasKnob, fuzzKnob, cookedKnob, dcTimingKnob;
    CentredSVGIcon tube;
};
