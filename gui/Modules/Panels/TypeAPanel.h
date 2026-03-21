#pragma once

#include "../Panel.h"


class TypeAPanel : public Panel
{
public:
    TypeAPanel(AudioPluginAudioProcessor &p) : Panel(p, "TYPE A"),
                                               threshold(p, "THRES", ParamIDs::TypeAThreshold, ParamUnits::db),
                                               brightness(p, "BRIGHT", ParamIDs::TypeARatio),
                                               out(p, "OUT", ParamIDs::TypeAOut, ParamUnits::db),
                                               tilt(p, "TILT", ParamIDs::TypeATilt, ParamUnits::db)
    {
        addAndMakeVisible(threshold);
        addAndMakeVisible(brightness);
        addAndMakeVisible(out);
        addAndMakeVisible(tilt);

        Palette::setKnobColoursOfComponent(&threshold, Palette::colours[3]);
        Palette::setKnobColoursOfComponent(&brightness, Palette::colours[3]);
        Palette::setKnobColoursOfComponent(&out, Palette::colours[3]);
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
            juce::GridItem(brightness).withArea(1, 2),
            juce::GridItem(tilt).withArea(2, 1),
            juce::GridItem(out).withArea(2, 2)};

        grid.performLayout(bounds);
    }

private:
    juce::Grid grid;

    ParamKnob threshold;
    ParamKnob brightness;
    ParamKnob out;
    ParamKnob tilt;
};