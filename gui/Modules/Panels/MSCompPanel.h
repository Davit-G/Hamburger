#pragma once

#include "../Panel.h"


class MSCompPanel : public Panel
{
public:
    MSCompPanel(AudioPluginAudioProcessor &p) : Panel(p, "MS", Palette::colours[3]),
                                                   threshold(p, "THRES", ParamIDs::MSCompThreshold, ParamUnits::db),
                                                   ratio(p, "RATIO", ParamIDs::compRatio),
                                                   tilt(p, "TILT", ParamIDs::compBandTilt, ParamUnits::db),
                                                   attack(p, "SPEED", ParamIDs::compSpeed, ParamUnits::ms),
                                                   makeup(p, "GAIN", ParamIDs::compOut, ParamUnits::db)
    {
        addAndMakeVisible(threshold);
        addAndMakeVisible(ratio);
        addAndMakeVisible(tilt);
        addAndMakeVisible(attack);
        addAndMakeVisible(makeup);

        Palette::setKnobColoursOfComponent(&threshold, Palette::colours[3]);
        Palette::setKnobColoursOfComponent(&ratio, Palette::colours[3]);
        Palette::setKnobColoursOfComponent(&tilt, Palette::colours[3]);
        Palette::setKnobColoursOfComponent(&attack, Palette::colours[3]);
        Palette::setKnobColoursOfComponent(&makeup, Palette::colours[3]);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        using fr = Grid::Fr;
        using Track = Grid::TrackInfo;

        grid.templateRows = {Track(fr(1)), Track(fr(1))}; // todo: optimise this
        grid.templateColumns = {Track(fr(1)), Track(fr(1)), Track(fr(1))};

        grid.items = {
            GridItem(threshold).withArea(1, 1),
            GridItem(tilt).withArea(1, 2),
            GridItem(ratio).withArea(1, 3),
            GridItem(attack).withArea(2, 1),
            GridItem(makeup).withArea(2, 3)};

        grid.performLayout(bounds);
    }

private:
    Grid grid;

    ParamKnob threshold;
    ParamKnob ratio;
    ParamKnob tilt;
    ParamKnob attack;
    ParamKnob makeup;
};