#pragma once

 
#include "../Panel.h"
#include "../../Knob.h"



class RubidiumSatPanel : public Panel
{
public:
    RubidiumSatPanel(AudioPluginAudioProcessor &p) : Panel(p, "RUBIDIUM", Palette::colours[2]), 
        tone(p, "TONE", ParamIDs::rubidiumTone, ParamUnits::hz, ScopeContextType::IN_OUT),
        mojo(p, "MOJO", ParamIDs::rubidiumMojo, ParamUnits::none, ScopeContextType::IN_OUT),
        hysteresis(p, "ASYM", ParamIDs::rubidiumAsym, ParamUnits::none, ScopeContextType::IN_OUT),
        drive(p, "DRIVE", ParamIDs::rubidiumAmount, ParamUnits::percent, ScopeContextType::IN_OUT),
        bias(p, "BIAS", ParamIDs::rubidiumBias, ParamUnits::none, ScopeContextType::IN_OUT),
        flask(BinaryData::Flask_svg, BinaryData::Flask_svgSize)
    {
        addAndMakeVisible(tone);
        addAndMakeVisible(drive);
        addAndMakeVisible(mojo);
        addAndMakeVisible(hysteresis);
        addAndMakeVisible(bias);
        addAndMakeVisible(flask);

        Palette::setKnobColoursOfComponent(&tone, Palette::colours[2]);
        Palette::setKnobColoursOfComponent(&drive, Palette::colours[2]);
        Palette::setKnobColoursOfComponent(&mojo, Palette::colours[2]);
        Palette::setKnobColoursOfComponent(&hysteresis, Palette::colours[2]);
        Palette::setKnobColoursOfComponent(&bias, Palette::colours[2]);
    }

    void resized() override
    {
        fiveKnobLayout(drive, flask, bias, hysteresis, mojo, tone);
    }

    ParamKnob drive;
    RectSlider tone;
    RectSlider mojo;
    RectSlider hysteresis;
    RectSlider bias;

    CentredSVGIcon flask;
};