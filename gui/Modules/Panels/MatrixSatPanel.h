#pragma once

#include "juce_gui_basics/juce_gui_basics.h"
#include "../Panel.h"
#include "../../Knob.h"



class MatrixSatPanel : public Panel
{
public:
    MatrixSatPanel(AudioPluginAudioProcessor &p) : Panel(p, "MATRIX"), 
        mat1(p, "DRIVE", ParamIDs::matrix1),
        mat2(p, "GRIME", ParamIDs::matrix2),
        mat3(p, "SHARP", ParamIDs::matrix3),
        mat4(p, "DIGI", ParamIDs::matrix4),
        mat5(p, "FILT MORPH", ParamIDs::matrix5),
        mat6(p, "FILT FREQ", ParamIDs::matrix6),
        mat7(p, "MAT7", ParamIDs::matrix7),
        mat8(p, "MAT8", ParamIDs::matrix8),
        mat9(p, "FILT MIX", ParamIDs::matrix9)
    {
        addAndMakeVisible(mat1);
        addAndMakeVisible(mat2);
        addAndMakeVisible(mat3);
        addAndMakeVisible(mat4);
        addAndMakeVisible(mat5);
        addAndMakeVisible(mat6);
        addAndMakeVisible(mat7);
        addAndMakeVisible(mat8);
        addAndMakeVisible(mat9);

        using fr = Grid::Fr;
        using Track = Grid::TrackInfo;

        grid.templateRows = {Track(fr(1)), Track(fr(1)), Track(fr(1))}; // todo: optimise this
        grid.templateColumns = {Track(fr(1)), Track(fr(1)), Track(fr(1))};

        grid.items = {
            GridItem(mat1).withArea(1, 1),
            GridItem(mat2).withArea(1, 2),
            GridItem(mat3).withArea(1, 3),
            GridItem(mat4).withArea(2, 1),
            GridItem(mat5).withArea(2, 2),
            GridItem(mat6).withArea(2, 3),
            GridItem(mat7).withArea(3, 1),
            GridItem(mat8).withArea(3, 2),
            GridItem(mat9).withArea(3, 3)
        };
            
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        grid.performLayout(bounds);
    }

private:
    juce::Grid grid;


    ParamKnob mat1;
    ParamKnob mat2;
    ParamKnob mat3;
    ParamKnob mat4;
    ParamKnob mat5;
    ParamKnob mat6;
    ParamKnob mat7;
    ParamKnob mat8;
    ParamKnob mat9;
};