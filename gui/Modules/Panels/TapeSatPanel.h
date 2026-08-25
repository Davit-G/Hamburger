#pragma once
 
#include "../Panel.h"
#include "../../Knob.h"

class TapeSatPanel : public Panel
{
public:
    TapeSatPanel(AudioPluginAudioProcessor &p) : Panel(p, "TAPE"), 
        drive(p, "DRIVE", ParamIDs::tapeDrive, ParamUnits::none, ScopeContextType::IN_OUT),
        bias(p, "DC BIAS", ParamIDs::tapeBias, ParamUnits::none, ScopeContextType::IN_OUT),
        tapeWidth(p, "AGE", ParamIDs::tapeWidth, ParamUnits::none, ScopeContextType::IN_OUT),
        reel(BinaryData::FilmReel_svg, BinaryData::FilmReel_svgSize)
    {
        addAndMakeVisible(drive);
        addAndMakeVisible(bias);
        addAndMakeVisible(tapeWidth);
        addAndMakeVisible(reel);

        Palette::setKnobColoursOfComponent(&drive, Palette::colours[7]);
        Palette::setKnobColoursOfComponent(&bias, Palette::colours[7]);
        Palette::setKnobColoursOfComponent(&tapeWidth, Palette::colours[7]);
    }

    void resized() override
    {
        threeKnobLayout(drive, reel, bias, tapeWidth);
    }

    ParamKnob drive;
    RectSlider tapeWidth;
    RectSlider bias;

    CentredSVGIcon reel;
};