#pragma once

#include "../Panel.h"

class ErosionPanel : public Panel
{
public:
    ErosionPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::module1, 0}) : Panel(p, "EROSION", Palette::colours[1]),
                                                 erosionAmt(p, "AMOUNT", slot, ParamIDs::erosionAmount, ValueType::none, ScopeContextType::NOISE),
                                                 erosionFreq(p, "FREQ", slot, ParamIDs::erosionFrequency, ValueType::hz, ScopeContextType::NOISE),
                                                 erosionQ(p, "Q", slot, ParamIDs::erosionQ, ValueType::none, ScopeContextType::NOISE)
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