#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"


class StereoCompPanel : public Panel
{
public:
    StereoCompPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::dynamics, 0}) : Panel(p, "STEREO", &Theme::stereoComp),
                                                   threshold(p, "THRES", slot, ParamIDs::stereoCompThreshold, ScopeContextType::COMPRESSION),
                                                   ratio(p, "RATIO", slot, ParamIDs::compRatio, ScopeContextType::COMPRESSION),
                                                   attack(p, "SPEED", slot, ParamIDs::compSpeed, ScopeContextType::COMPRESSION),
                                                   link(p, "LINK", slot, ParamIDs::compStereoLink, ScopeContextType::COMPRESSION)
    {
        addAndMakeVisible(threshold);
        addAndMakeVisible(ratio);
        addAndMakeVisible(attack);
        addAndMakeVisible(link);
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
            juce::GridItem(ratio).withArea(1, 2),
            juce::GridItem(attack).withArea(2, 1),
            juce::GridItem(link).withArea(2, 2)};

        grid.performLayout(bounds);
    }

private:
    juce::Grid grid;

    ParamKnob threshold;
    ParamKnob ratio;
    ParamKnob attack;
    ParamKnob link;
};