#pragma once
 
#include "../Panel.h"
#include "../../Knob.h"

class PreisachPanel : public Panel
{
public:
    PreisachPanel(AudioPluginAudioProcessor &p) : Panel(p, "PREISACH"), 
        drive(p, "DRIVE", ParamIDs::preisachDrive),
        coerc(p, "COERCIVITY", ParamIDs::preisachCoercivity),
        rem(p, "REMANENCE", ParamIDs::preisachRemanence)
    {
        addAndMakeVisible(drive);
        addAndMakeVisible(coerc);
        addAndMakeVisible(rem);

        Palette::setKnobColoursOfComponent(&drive, Palette::colours[4]);
        Palette::setKnobColoursOfComponent(&coerc, Palette::colours[4]);
        Palette::setKnobColoursOfComponent(&rem, Palette::colours[4]);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        drive.setBounds(bounds.removeFromTop(static_cast<int>(bounds.getHeight() * 0.6666f)).reduced(10));

        auto width = bounds.getWidth() / 2;
        coerc.setBounds(bounds.removeFromLeft(width));
        rem.setBounds(bounds);
    }

    ParamKnob drive;
    ParamKnob coerc;
    ParamKnob rem;
};