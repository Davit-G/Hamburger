#pragma once

 
#include "../Panel.h"
#include "../../Knob.h"
#include "../../RectSlider.h"

class ClassicSatPanel : public Panel
{
public:
    ClassicSatPanel(AudioPluginAudioProcessor &p) : Panel(p, "GRILL", Palette::colours[0]), 
        satKnob(p, "SATURATION", "saturationAmount", ParamUnits::percent, ScopeContextType::IN_OUT),
        biasKnob(p, "DC BIAS", ParamIDs::grillBias, ParamUnits::none, ScopeContextType::IN_OUT),
        fuzzKnob(p, "DIODE", ParamIDs::diode, ParamUnits::none, ScopeContextType::IN_OUT),
        cookedKnob(p, "WAVEFOLD", ParamIDs::fold, ParamUnits::none, ScopeContextType::IN_OUT)
    {
        addAndMakeVisible(satKnob);
        addAndMakeVisible(biasKnob);
        addAndMakeVisible(fuzzKnob);
        addAndMakeVisible(cookedKnob);

        Palette::setKnobColoursOfComponent(&satKnob, Palette::colours[0]);
        Palette::setKnobColoursOfComponent(&biasKnob, Palette::colours[0]);
        Palette::setKnobColoursOfComponent(&fuzzKnob, Palette::colours[0]);
        Palette::setKnobColoursOfComponent(&cookedKnob, Palette::colours[0]);

        cookedKnob.setJustification(RectSliderType::CenterJustifified);
        biasKnob.setJustification(RectSliderType::RightJustifified);
        fuzzKnob.setJustification(RectSliderType::LeftJustifified);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        satKnob.setBounds(bounds.removeFromTop(bounds.getHeight() / 1.4f).reduced(5.0f));

        bounds.reduce(20, 0);
        auto width = bounds.getWidth() / 3;

        biasKnob.setBounds(bounds.removeFromLeft(width).withTrimmedBottom(2 * bounds.getHeight() / 3));
        cookedKnob.setBounds(bounds.removeFromLeft(width).withTrimmedTop(bounds.getHeight() / 3).withTrimmedBottom(bounds.getHeight() / 3));
        fuzzKnob.setBounds(bounds.withTrimmedBottom(2 * bounds.getHeight() / 3));
    }

private:
    ParamKnob satKnob;
    RectSlider cookedKnob;
    RectSlider biasKnob;
    RectSlider fuzzKnob;
};