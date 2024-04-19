#pragma once

 
#include "../Panel.h"
#include "../../Knob.h"


class ClassicSatPanel : public Panel
{
public:
    ClassicSatPanel(AudioPluginAudioProcessor &p) : Panel(p, "GRILL"), 
        satKnob(p, "SATURATION", "saturationAmount", ParamUnits::percent),
        biasKnob(p, "BIAS", ParamIDs::grillBias),
        fuzzKnob(p, "DIODE", ParamIDs::diode),
        cookedKnob(p, "FOLD", ParamIDs::fold)
    {
        addAndMakeVisible(satKnob);
        addAndMakeVisible(biasKnob);
        addAndMakeVisible(fuzzKnob);
        addAndMakeVisible(cookedKnob);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        satKnob.setBounds(bounds.removeFromTop(bounds.getHeight() / 1.5f).reduced(10));

        auto width = bounds.getWidth() / 3;
        biasKnob.setBounds(bounds.removeFromLeft(width));
        cookedKnob.setBounds(bounds.removeFromLeft(width));
        fuzzKnob.setBounds(bounds);
    }

private:
    ParamKnob satKnob;
    ParamKnob cookedKnob;
    ParamKnob biasKnob;
    ParamKnob fuzzKnob;
};