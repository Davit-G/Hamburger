#pragma once

#include <JuceHeader.h>

#include "../Panel.h"
#include "PanelNames.h"

class CompanderPanel : public Panel
{
public:
    CompanderPanel(AudioPluginAudioProcessor &p) : Panel(p, "COMPANDER"),
                                                   threshold(p, "THRESHOLD", "compThreshold", ParamUnits::db),
                                                   ratio(p, "RATIO", "compRatio"),
                                                   attack(p, "SPEED", "compSpeed", ParamUnits::ms),
                                                   makeup(p, "GAIN", "compOut", ParamUnits::db)
    {
        auto laf = new KnobLAF(juce::Colours::blueviolet);
        setLookAndFeel(laf); // will cascade to all children knobs

        addAndMakeVisible(threshold);
        addAndMakeVisible(ratio);
        addAndMakeVisible(attack);
        addAndMakeVisible(makeup);
    }

    // void paint(juce::Graphics &g) override
    // {
    //     // g.fillAll(juce::Colours::green);
    // }

    void resized() override
    {
        // knob.setBounds(getLocalBounds());
        auto bounds = getLocalBounds();

        using fr = Grid::Fr;
        using Track = Grid::TrackInfo;

        grid.templateRows = {Track(fr(1)), Track(fr(1))};
        grid.templateColumns = {Track(fr(1)), Track(fr(1)), Track(fr(1))};

        grid.items = {
            GridItem(threshold).withArea(1, 1),
            // GridItem(knee).withArea(1, 2),
            GridItem(ratio).withArea(1, 2),
            GridItem(attack).withArea(2, 1),
            GridItem(makeup).withArea(2, 3)};

        grid.performLayout(bounds);
    }

private:
    Grid grid;

    ParamKnob threshold;
    ParamKnob ratio;
    ParamKnob attack;
    ParamKnob makeup;
};