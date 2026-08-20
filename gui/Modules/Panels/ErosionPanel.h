#pragma once

#include "../Panel.h"

class ErosionPanel : public Panel
{
public:
    ErosionPanel(AudioPluginAudioProcessor &p) : Panel(p, "EROSION", Palette::colours[1]),
                                                 erosionAmt(p, "AMOUNT", ParamIDs::erosionAmount, ParamUnits::none, ScopeContextType::NOISE),
                                                 erosionFreq(p, "FREQ", ParamIDs::erosionFrequency, ParamUnits::hz, ScopeContextType::NOISE),
                                                 erosionQ(p, "Q", ParamIDs::erosionQ, ParamUnits::none, ScopeContextType::NOISE)
    {

        addAndMakeVisible(erosionAmt);
        addAndMakeVisible(erosionFreq);
        addAndMakeVisible(erosionQ);

        Palette::setKnobColoursOfComponent(&erosionAmt, Palette::colours[1]);
        Palette::setKnobColoursOfComponent(&erosionFreq, Palette::colours[1]);
        Palette::setKnobColoursOfComponent(&erosionQ, Palette::colours[1]);
    }

    void resized()
    {
        // three, in a row
        auto bounds = getLocalBounds();
        erosionAmt.setBounds(bounds.removeFromLeft(bounds.getWidth() / 3));
        erosionFreq.setBounds(bounds.removeFromLeft(bounds.getWidth() / 2));
        erosionQ.setBounds(bounds);
    }

    ParamKnob erosionAmt;
    ParamKnob erosionFreq;
    ParamKnob erosionQ;
};