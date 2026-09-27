#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"

class MBCompPanel : public Panel
{
public:
    MBCompPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::dynamics, 0}) : Panel(p, "MB", &Theme::multibandComp),
                                                   threshold(p, "THRES", slot, ParamIDs::MBCompThreshold, ScopeContextType::COMPRESSION),
                                                   ratio(p, "RATIO", slot, ParamIDs::compRatio, ScopeContextType::COMPRESSION),
                                                   tilt(p, "TILT", slot, ParamIDs::compBandTilt, ScopeContextType::COMPRESSION),
                                                   attack(p, "SPEED", slot, ParamIDs::MBCompSpeed, ScopeContextType::COMPRESSION)
    {
        addAndMakeVisible(threshold);
        addAndMakeVisible(ratio);
        addAndMakeVisible(tilt);
        addAndMakeVisible(attack);
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
            juce::GridItem(tilt).withArea(2, 2)};

        grid.performLayout(bounds);
    }

private:
    juce::Grid grid;

    ParamKnob threshold;
    ParamKnob ratio;
    ParamKnob tilt;
    ParamKnob attack;
};