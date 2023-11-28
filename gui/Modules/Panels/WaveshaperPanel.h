#pragma once

#include "../Panel.h"
#include "../../Knob.h"
#include "PanelNames.h"

class WaveshaperPanel : public Panel
{
public:
    WaveshaperPanel(AudioPluginAudioProcessor &p) : Panel(p, "WAVESHAPER"), 
        satKnob(p, "SATURATION", "saturationAmount", ParamUnits::percent),
        biasKnob(p, "FOLD", "bias"),
        fuzzKnob(p, "ETC", "fuzz")
    {
        addAndMakeVisible(satKnob);
        addAndMakeVisible(biasKnob);
        addAndMakeVisible(fuzzKnob);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        satKnob.setBounds(bounds.removeFromTop(bounds.getHeight() / 1.5f).reduced(10));

        auto width = bounds.getWidth() / 4;
        biasKnob.setBounds(bounds.removeFromLeft(width));
        fuzzKnob.setBounds(bounds);
    }

private:
    ParamKnob satKnob;
    ParamKnob biasKnob;
    ParamKnob fuzzKnob;
};