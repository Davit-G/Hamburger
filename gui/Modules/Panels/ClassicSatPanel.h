#pragma once

#include "../Panel.h"
#include "../../SaturationIcons/CentredSVGIcon.h"

class ClassicSatPanel : public Panel
{
public:
    ClassicSatPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::main, 0}) : Panel(p, "GRILL", Palette::colours[0]),
        satKnob(p, "SATURATION", slot, ParamIDs::saturationAmount, ValueType::percent, ScopeContextType::IN_OUT),
        biasKnob(p, "DC BIAS", slot, ParamIDs::grillBias, ValueType::none, ScopeContextType::IN_OUT),
        fuzzKnob(p, "DIODE", slot, ParamIDs::diode, ValueType::none, ScopeContextType::IN_OUT),
        cookedKnob(p, "WAVEFOLD", slot, ParamIDs::fold, ValueType::none, ScopeContextType::IN_OUT),
        tube(BinaryData::Grill_svg, BinaryData::Grill_svgSize, -3)
    {
        addKnobs({&satKnob, &biasKnob, &fuzzKnob, &cookedKnob});
        addAndMakeVisible(tube);
    }

    void resized() override { layoutSaturation(satKnob, tube, {&biasKnob, &cookedKnob, &fuzzKnob}); }

private:
    ParamKnob satKnob;
    RectSlider biasKnob, fuzzKnob, cookedKnob;
    CentredSVGIcon tube;
};
