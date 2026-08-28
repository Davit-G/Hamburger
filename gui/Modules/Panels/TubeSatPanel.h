#pragma once

 
#include "../Panel.h"
#include "../../Knob.h"




class TubeSatPanel : public Panel
{
public:
    TubeSatPanel(AudioPluginAudioProcessor &p) : Panel(p, "TUBE"), 
        tubeTone(p, "TUBE TONE", ParamIDs::tubeTone, ParamUnits::none, ScopeContextType::IN_OUT),
        bias(p, "BIAS", ParamIDs::tubeBias, ParamUnits::none, ScopeContextType::IN_OUT),
        jeff(p, "JEFF", ParamIDs::jeffAmount, ParamUnits::none, ScopeContextType::IN_OUT),
        drive(p, "DRIVE", ParamIDs::tubeAmount, ParamUnits::percent, ScopeContextType::IN_OUT),
        tube(BinaryData::Tube_svg, BinaryData::Tube_svgSize)
    {
        addAndMakeVisible(tubeTone);
        addAndMakeVisible(drive);
        addAndMakeVisible(bias);
        addAndMakeVisible(jeff);
        addAndMakeVisible(tube);

        Palette::setKnobColoursOfComponent(&tubeTone, Palette::colours[4]);
        Palette::setKnobColoursOfComponent(&drive, Palette::colours[4]);
        Palette::setKnobColoursOfComponent(&bias, Palette::colours[4]);
        Palette::setKnobColoursOfComponent(&jeff, Palette::colours[4]);
    }

    void resized() override
    {
        fourKnobLayout(drive, tube, bias, jeff, tubeTone);
    }

    RectSlider tubeTone;
    ParamKnob drive;
    RectSlider jeff;
    RectSlider bias;

    CentredSVGIcon tube;
};