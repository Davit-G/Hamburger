#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"


class MSCompPanel : public Panel
{
public:
    MSCompPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::dynamics, 0}) : Panel(p, "MS", &Theme::midSideComp),
                                                   threshold(p, "THRES", slot, ParamIDs::MSCompThreshold, ScopeContextType::COMPRESSION),
                                                   ratio(p, "RATIO", slot, ParamIDs::compRatio, ScopeContextType::COMPRESSION),
                                                   tilt(p, "TILT", slot, ParamIDs::compBandTilt, ScopeContextType::COMPRESSION),
                                                   attack(p, "SPEED", slot, ParamIDs::MSCompSpeed, ScopeContextType::COMPRESSION),
                                                   makeup(p, "GAIN", slot, ParamIDs::compOut, ScopeContextType::COMPRESSION)
    {
        addAndMakeVisible(threshold);
        addAndMakeVisible(ratio);
        addAndMakeVisible(tilt);
        addAndMakeVisible(attack);
        addAndMakeVisible(makeup);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        using fr = juce::Grid::Fr;
        using Track = juce::Grid::TrackInfo;

        grid.templateRows = {Track(fr(1)), Track(fr(1))}; // todo: optimise this
        grid.templateColumns = {Track(fr(1)), Track(fr(1)), Track(fr(1))};

        grid.items = {
            juce::GridItem(threshold).withArea(1, 1),
            juce::GridItem(tilt).withArea(1, 2),
            juce::GridItem(ratio).withArea(1, 3),
            juce::GridItem(attack).withArea(2, 1),
            juce::GridItem(makeup).withArea(2, 3)};

        grid.performLayout(bounds);
    }

private:
    juce::Grid grid;

    ParamKnob threshold;
    ParamKnob ratio;
    ParamKnob tilt;
    ParamKnob attack;
    ParamKnob makeup;
};