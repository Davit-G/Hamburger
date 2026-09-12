#pragma once

 
#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"
#include "../../Knob.h"
#include "../../RectSlider.h"

#include "../../SaturationIcons/CentredSVGIcon.h"

class ClassicSatPanel : public Panel
{
public:
    ClassicSatPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::main, 0}) : Panel(p, "GRILL", Palette::colours[0]), 
        satKnob(p, "SATURATION", slot, ParamIDs::saturationAmount, ScopeContextType::IN_OUT),
        biasKnob(p, "DC BIAS", slot, ParamIDs::grillBias, ScopeContextType::IN_OUT),
        fuzzKnob(p, "DIODE", slot, ParamIDs::diode, ScopeContextType::IN_OUT),
        cookedKnob(p, "WAVEFOLD", slot, ParamIDs::fold, ScopeContextType::IN_OUT),
        tube(BinaryData::Grill_svg, BinaryData::Grill_svgSize, -3)
    {
        addAndMakeVisible(satKnob);
        addAndMakeVisible(biasKnob);
        addAndMakeVisible(fuzzKnob);
        addAndMakeVisible(cookedKnob);
        addAndMakeVisible(tube);

        Palette::setKnobColoursOfComponent(&satKnob, Palette::colours[0]);
        Palette::setKnobColoursOfComponent(&biasKnob, Palette::colours[0]);
        Palette::setKnobColoursOfComponent(&fuzzKnob, Palette::colours[0]);
        Palette::setKnobColoursOfComponent(&cookedKnob, Palette::colours[0]);
    }

    void resized() override
    {
        fourKnobLayout(satKnob, tube, biasKnob, cookedKnob, fuzzKnob);
    }

private:
    ParamKnob satKnob;
    RectSlider cookedKnob;
    RectSlider biasKnob;
    RectSlider fuzzKnob;

    CentredSVGIcon tube;
};