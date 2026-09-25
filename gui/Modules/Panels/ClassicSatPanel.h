#pragma once

 
#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"
#include "../../Knob.h"
#include "../../RectSlider.h"

#include "../../SaturationIcons/CentredSVGIcon.h"

class ClassicSatPanel : public Panel
{
public:
    ClassicSatPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::main, 0}) : Panel(p, "GRILL", Palette::colours[0], slot), 
        satKnob(p, "SATURATION", slot, ParamIDs::saturationAmount, ScopeContextType::IN_OUT),
        biasKnob(p, "DC BIAS", slot, ParamIDs::grillBias, ScopeContextType::IN_OUT),
        fuzzKnob(p, "DIODE", slot, ParamIDs::diode, ScopeContextType::IN_OUT),
        cookedKnob(p, "WAVEFOLD", slot, ParamIDs::fold, ScopeContextType::IN_OUT),
        tube(BinaryData::Grill_svg, BinaryData::Grill_svgSize, -3),
        dcTimingKnob(p, "DC SLEW", slot, ParamIDs::grillDcTiming, ScopeContextType::IN_OUT)
    {
        addAndMakeVisible(satKnob);
        addAndMakeVisible(biasKnob);
        addAndMakeVisible(fuzzKnob);
        addAndMakeVisible(cookedKnob);
        addAndMakeVisible(tube);
        addAndMakeVisible(dcTimingKnob);

        const auto colour = Palette::colours[0];
        Palette::setKnobColoursOfComponent(this, colour);

        Palette::setKnobColoursOfComponent(&satKnob, colour);
        Palette::setKnobColoursOfComponent(&biasKnob, colour);
        Palette::setKnobColoursOfComponent(&fuzzKnob, colour);
        Palette::setKnobColoursOfComponent(&cookedKnob, colour);
        Palette::setKnobColoursOfComponent(&dcTimingKnob, colour);
    }

    void resized() override
    {
        // the timing sits under the bias it times, wavefold and diode on the other side
        if (usesCompactLayout())
            compactLayout(satKnob, tube, {&biasKnob, &dcTimingKnob, &cookedKnob, &fuzzKnob});
        else
            fiveKnobLayout(satKnob, tube, biasKnob, dcTimingKnob, cookedKnob, fuzzKnob);
    }

private:
    ParamKnob satKnob;
    RectSlider cookedKnob;
    RectSlider biasKnob;
    RectSlider fuzzKnob;

    CentredSVGIcon tube;

    RectSlider dcTimingKnob;
};