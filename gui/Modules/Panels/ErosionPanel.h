#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"

class ErosionPanel : public Panel
{
public:
    ErosionPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::module1, 0}) : Panel(p, "EROSION", Palette::colours[1]),
                                                 erosionAmt(p, "AMOUNT", slot, ParamIDs::erosionAmount, ScopeContextType::NOISE),
                                                 erosionFreq(p, "FREQ", slot, ParamIDs::erosionFrequency, ScopeContextType::NOISE),
                                                 erosionQ(p, "Q", slot, ParamIDs::erosionQ, ScopeContextType::NOISE)
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