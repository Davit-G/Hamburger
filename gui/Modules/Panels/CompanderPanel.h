#pragma once

#include <JuceHeader.h>

#include "../Panel.h"
#include "PanelNames.h"

class CompanderPanel : public Panel
{
public:
    CompanderPanel(AudioPluginAudioProcessor &p) : Panel(p, "COMPANDER"),
                                                   threshold(p, "THRESHOLD", "compThreshold", -48.0f, 0.0f),
                                                   ratio(p, "RATIO", "compRatio", 1.0f, 10.0f),
                                                   attack(p, "ATTACK", "compAttack", 3.0f, 200.0f),
                                                   release(p, "RELEASE", "compRelease", 10.0f, 500.0f),
                                                   makeup(p, "GAIN", "compOut", 0.0f, 24.0f)
    {
        auto laf = new KnobLAF(juce::Colours::blueviolet);
        setLookAndFeel(laf); // will cascade to all children knobs

        addAndMakeVisible(threshold);
        addAndMakeVisible(ratio);
        addAndMakeVisible(attack);
        addAndMakeVisible(release);
        addAndMakeVisible(makeup);
    }

    void paint(juce::Graphics &g) override
    {
        // g.fillAll(juce::Colours::green);
    }

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
            GridItem(release).withArea(2, 2),
            GridItem(makeup).withArea(2, 3)};

        grid.performLayout(bounds);
    }

private:
    Grid grid;

    ParamKnob threshold;
    // ParamKnob knee;
    ParamKnob ratio;
    ParamKnob attack;
    ParamKnob release;
    ParamKnob makeup;
};