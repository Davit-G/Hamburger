#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"


class StereoCompPanel : public Panel
{
public:
    StereoCompPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::dynamics, 0}) : Panel(p, "STEREO", &Theme::stereoComp),
                                                   threshold(p, "THRES", slot, ParamIDs::stereoCompThreshold, ScopeContextType::COMPRESSION),
                                                   ratio(p, "RATIO", slot, ParamIDs::compRatio, ScopeContextType::COMPRESSION),
                                                //    tilt(p, "S-LNK", "compStereoLink"),
                                                   attack(p, "SPEED", slot, ParamIDs::compSpeed, ScopeContextType::COMPRESSION),
                                                   makeup(p, "GAIN", slot, ParamIDs::compOut, ScopeContextType::COMPRESSION)
    {
        addAndMakeVisible(threshold);
        addAndMakeVisible(ratio);
        // addAndMakeVisible(tilt);
        addAndMakeVisible(attack);
        addAndMakeVisible(makeup);
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
            juce::GridItem(attack).withArea(2, 1),
            juce::GridItem(makeup).withArea(2, 2)};

        grid.performLayout(bounds);
    }

private:
    juce::Grid grid;

    ParamKnob threshold;
    ParamKnob ratio;
    // ParamKnob tilt;
    ParamKnob attack;
    ParamKnob makeup;
};