#pragma once

 
#include "../Panel.h"
#include "../../Knob.h"
#include "../../RectSlider.h"

#include "../../SaturationIcons/CentredSVGIcon.h"

class ClassicSatPanel : public Panel
{
public:
    ClassicSatPanel(AudioPluginAudioProcessor &p) : Panel(p, "GRILL", Palette::colours[0]), 
        satKnob(p, "SATURATION", "saturationAmount", ParamUnits::percent, ScopeContextType::IN_OUT),
        biasKnob(p, "DC BIAS", ParamIDs::grillBias, ParamUnits::none, ScopeContextType::IN_OUT),
        fuzzKnob(p, "DIODE", ParamIDs::diode, ParamUnits::none, ScopeContextType::IN_OUT),
        cookedKnob(p, "WAVEFOLD", ParamIDs::fold, ParamUnits::none, ScopeContextType::IN_OUT),
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