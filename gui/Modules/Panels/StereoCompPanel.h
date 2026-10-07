#pragma once

#include "../Panel.h"


class StereoCompPanel : public Panel
{
public:
    StereoCompPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::dynamics, 0}) : Panel(p, ""),
                                                   threshold(p, "THRES", slot, ParamIDs::stereoCompThreshold, ValueType::db, ScopeContextType::COMPRESSION),
                                                   ratio(p, "RATIO", slot, ParamIDs::compRatio, ValueType::compressionRatio, ScopeContextType::COMPRESSION),
                                                //    tilt(p, "S-LNK", "compStereoLink", ValueType::percent),
                                                   attack(p, "SPEED", slot, ParamIDs::compSpeed, ValueType::ms, ScopeContextType::COMPRESSION)
    {
        addAndMakeVisible(threshold);
        addAndMakeVisible(ratio);
        // addAndMakeVisible(tilt);
        addAndMakeVisible(attack);

        Palette::setKnobColoursOfComponent(&threshold, Palette::colours[3]);
        Palette::setKnobColoursOfComponent(&ratio, Palette::colours[3]);
        // Palette::setKnobColoursOfComponent(&tilt, Palette::colours[3]);
        Palette::setKnobColoursOfComponent(&attack, Palette::colours[3]);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        using fr = juce::Grid::Fr;
        using Track = juce::Grid::TrackInfo;

        grid.templateRows = {Track(fr(1)), Track(fr(1))};
        grid.templateColumns = {Track(fr(1)), Track(fr(1))};

        grid.items = {
            juce::GridItem(threshold).withArea(1, 1),
            // juce::GridItem(tilt).withArea(1, 2),
            juce::GridItem(ratio).withArea(1, 2),
            juce::GridItem(attack).withArea(2, 1)};

        grid.performLayout(bounds);
    }

private:
    juce::Grid grid;

    ParamKnob threshold;
    ParamKnob ratio;
    // ParamKnob tilt;
    ParamKnob attack;
};