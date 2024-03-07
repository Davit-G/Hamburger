#pragma once

#include "../Panel.h"

class EQPanel : public Panel
{
public:
    EQPanel(AudioPluginAudioProcessor &p) : Panel(p, "EMPHASIS"),
                                            lowFreq(p, "LOW", "emphasisLowFreq", ParamUnits::hz),
                                             midFreq(p, "MID", "emphasisMidFreq", ParamUnits::hz),
                                            highFreq(p, "HIGH", "emphasisHighFreq", ParamUnits::hz),
                                            lowGain(p, "GAIN", "emphasisLowGain", ParamUnits::db),
                                            midGain(p, "GAIN", "emphasisMidGain", ParamUnits::db),
                                            highGain(p, "GAIN", "emphasisHighGain", ParamUnits::db)
    {

        addAndMakeVisible(lowFreq);
        addAndMakeVisible(midFreq);
        addAndMakeVisible(highFreq);
        addAndMakeVisible(lowGain);
        addAndMakeVisible(midGain); //
        addAndMakeVisible(highGain);
    }

    void resized() {
        auto bounds = getLocalBounds();

        using fr = Grid::Fr;
        using Track = Grid::TrackInfo;

        grid.templateRows = {Track(fr(1)), Track(fr(1))};
        grid.templateColumns = {Track(fr(1)), Track(fr(1)), Track(fr(1))};

        grid.items = {
            GridItem(lowFreq).withArea(1, 1),
            GridItem(midFreq).withArea(1, 2),
            GridItem(highFreq).withArea(1, 3),
            GridItem(lowGain).withArea(2, 1),
            GridItem(midGain).withArea(2, 2),
            GridItem(highGain).withArea(2, 3)
        };

        grid.performLayout(bounds);
    }

private:
    Grid grid;

    ParamKnob lowFreq;
    ParamKnob midFreq;
    ParamKnob highFreq;
    ParamKnob lowGain;
    ParamKnob midGain;
    ParamKnob highGain;
};