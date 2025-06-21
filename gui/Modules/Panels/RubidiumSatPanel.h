#pragma once

 
#include "../Panel.h"
#include "../../Knob.h"



class RubidiumSatPanel : public Panel
{
public:
    RubidiumSatPanel(AudioPluginAudioProcessor &p) : Panel(p, "RUBIDIUM", Palette::colours[2]), 
        tubeTone(p, "TONE", ParamIDs::rubidiumTone, ParamUnits::hz),
        mojo(p, "MOJO", ParamIDs::rubidiumMojo),
        hysteresis(p, "ASYM", ParamIDs::rubidiumAsym),
        drive(p, "DRIVE", ParamIDs::rubidiumAmount, ParamUnits::percent),
        bias(p, "BIAS", ParamIDs::rubidiumBias)
    {
        addAndMakeVisible(tubeTone);
        addAndMakeVisible(drive);
        addAndMakeVisible(mojo);
        addAndMakeVisible(hysteresis);
        addAndMakeVisible(bias);

        Palette::setKnobColoursOfComponent(&tubeTone, Palette::colours[2]);
        Palette::setKnobColoursOfComponent(&drive, Palette::colours[2]);
        Palette::setKnobColoursOfComponent(&mojo, Palette::colours[2]);
        Palette::setKnobColoursOfComponent(&hysteresis, Palette::colours[2]);
        Palette::setKnobColoursOfComponent(&bias, Palette::colours[2]);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        drive.setBounds(bounds.removeFromTop(bounds.getHeight() / 1.5f).reduced(10));

        auto width = bounds.getWidth() / 4;
        tubeTone.setBounds(bounds.removeFromLeft(width));
        mojo.setBounds(bounds.removeFromLeft(width));
        bias.setBounds(bounds.removeFromLeft(width));
        hysteresis.setBounds(bounds);
    }

    ParamKnob tubeTone;
    ParamKnob drive;
    ParamKnob mojo;
    ParamKnob hysteresis;
    ParamKnob bias;
};