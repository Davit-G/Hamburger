#pragma once

#include <JuceHeader.h>
#include "../Panel.h"
#include "../../Knob.h"
#include "PanelNames.h"

class ClassicSatPanel : public Panel
{
public:
    ClassicSatPanel(AudioPluginAudioProcessor &p) : Panel(p, "CLASSIC"), 
        satKnob(p, "SATURATION", "saturationAmount", 0.0f, 100.0f),
        foldKnob(p, "FOLD", "fold", 0.0f, 100.0f),
        biasKnob(p, "BIAS", "bias", 0.0f, 100.0f),
        fuzzKnob(p, "FUZZ", "fuzz", 0.0f, 100.0f)
        {
        
        addAndMakeVisible(satKnob);
        addAndMakeVisible(foldKnob);
        addAndMakeVisible(biasKnob);
        addAndMakeVisible(fuzzKnob);

        auto laf = new KnobLAF(juce::Colours::orangered);
        setLookAndFeel(laf); // will cascade to all children knobs
    }

    void paint(juce::Graphics &g) override
    {
        // g.fillAll(juce::Colours::green);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        satKnob.setBounds(bounds.removeFromTop(bounds.getHeight() / 1.5f).reduced(10));

        auto width = bounds.getWidth() / 3;
        foldKnob.setBounds(bounds.removeFromLeft(width));
        bounds.removeFromLeft(3);
        biasKnob.setBounds(bounds.removeFromLeft(width));
        bounds.removeFromRight(3);
        fuzzKnob.setBounds(bounds);
    }

    ParamKnob satKnob;
    ParamKnob foldKnob;
    ParamKnob biasKnob;
    ParamKnob fuzzKnob;
};