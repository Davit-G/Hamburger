#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"


class TypeAPanel : public Panel
{
public:
    TypeAPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::dynamics, 0}) : Panel(p, "TYPE A", &Theme::typeAComp),
                                               threshold(p, "THRES", slot, ParamIDs::TypeAThreshold, ScopeContextType::COMPRESSION),
                                               speed(p, "SPEED", slot, ParamIDs::TypeACompSpeed, ScopeContextType::COMPRESSION),
                                               out(p, "OUT", slot, ParamIDs::TypeAOut, ScopeContextType::COMPRESSION),
                                               tilt(p, "TILT", slot, ParamIDs::TypeATilt, ScopeContextType::COMPRESSION)
    {
        addAndMakeVisible(threshold);
        addAndMakeVisible(speed);
        addAndMakeVisible(out);
        addAndMakeVisible(tilt);
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
            juce::GridItem(tilt).withArea(1, 2),
            juce::GridItem(speed).withArea(2, 1),
            juce::GridItem(out).withArea(2, 2)};

        grid.performLayout(bounds);
    }

private:
    juce::Grid grid;

    ParamKnob threshold;
    ParamKnob speed;
    ParamKnob out;
    ParamKnob tilt;
};