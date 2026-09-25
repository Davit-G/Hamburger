#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"

class ErosionPanel : public Panel
{
public:
    ErosionPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::module1, 0}) : Panel(p, "EROSION", &Theme::erosion),
                                                 erosionAmt(p, "AMOUNT", slot, ParamIDs::erosionAmount, ScopeContextType::NOISE),
                                                 erosionFreq(p, "FREQ", slot, ParamIDs::erosionFrequency, ScopeContextType::NOISE),
                                                 erosionQ(p, "Q", slot, ParamIDs::erosionQ, ScopeContextType::NOISE)
    {

        addAndMakeVisible(erosionAmt);
        addAndMakeVisible(erosionFreq);
        addAndMakeVisible(erosionQ);
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