#pragma once

#include <JuceHeader.h>

#include "../Panel.h"
#include "../../LookAndFeel/KnobLAF.h"

class EQPanel : public Panel
{
public:
    EQPanel(AudioPluginAudioProcessor &p) : Panel(p, "EMPHASIS"),
                                            lowFreq(p, "FREQ", "emphasisLowFreq", 30.0f, 200.0f),
                                            midFreq(p, "FREQ", "emphasisMidFreq", 500.0f, 3000.0f),
                                            highFreq(p, "FREQ", "emphasisHighFreq", 6000.0f, 18000.0f),
                                            lowGain(p, "GAIN", "emphasisLowGain", -18.0f, 18.0f),
                                            midGain(p, "GAIN", "emphasisMidGain", -18.0f, 18.0f),
                                            highGain(p, "GAIN", "emphasisHighGain", -18.0f, 18.0f)
    {
        auto laf = new KnobLAF(juce::Colours::orange);
        setLookAndFeel(laf); // will cascade to all children knobs

        addAndMakeVisible(lowFreq);
        addAndMakeVisible(midFreq);
        addAndMakeVisible(highFreq);
        addAndMakeVisible(lowGain);
        addAndMakeVisible(midGain);
        addAndMakeVisible(highGain);
    }

    void paint(juce::Graphics &g) override
    {
    }

    void resized() {
        // knob.setBounds(getLocalBounds());
        auto bounds = getLocalBounds();

        using fr = Grid::Fr;
        using Track = Grid::TrackInfo;

        grid.templateRows = {Track(fr(1)), Track(fr(1))};
        grid.templateColumns = {Track(fr(1)), Track(fr(1)), Track(fr(1))};

        grid.items = {
            GridItem(lowFreq).withArea(1, 1),
            GridItem(midFreq).withArea(1, 2),
            GridItem(highFreq).withArea(1, 3),
            GridItem(lowGain).withArea(2, 1),
            GridItem(midGain).withArea(2, 2),
            GridItem(highGain).withArea(2, 3)
        };

        grid.performLayout(bounds);
    }

private:
    Grid grid;

    ParamKnob lowFreq;
    ParamKnob midFreq;
    ParamKnob highFreq;
    ParamKnob lowGain;
    ParamKnob midGain;
    ParamKnob highGain;
};