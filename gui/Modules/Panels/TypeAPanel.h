#pragma once

#include "../Panel.h"


class TypeAPanel : public Panel
{
public:
    TypeAPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::dynamics, 0}) : Panel(p, "TYPE A", Palette::colours[3]),
                                               threshold(p, "THRES", slot, ParamIDs::TypeAThreshold, ValueType::db, ScopeContextType::COMPRESSION),
                                               speed(p, "SPEED", slot, ParamIDs::compSpeed, ValueType::ms, ScopeContextType::COMPRESSION),
                                               tilt(p, "TILT", slot, ParamIDs::TypeATilt, ValueType::db, ScopeContextType::COMPRESSION)
    {
        addAndMakeVisible(threshold);
        addAndMakeVisible(speed);
        addAndMakeVisible(tilt);

        Palette::setKnobColoursOfComponent(&threshold, Palette::colours[3]);
        Palette::setKnobColoursOfComponent(&speed, Palette::colours[3]);
        Palette::setKnobColoursOfComponent(&tilt, Palette::colours[3]);
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
            juce::GridItem(speed).withArea(2, 1)};

        grid.performLayout(bounds);
    }

private:
    juce::Grid grid;

    ParamKnob threshold;
    ParamKnob speed;
    ParamKnob tilt;
};