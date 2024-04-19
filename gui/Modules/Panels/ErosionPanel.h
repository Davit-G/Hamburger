#pragma once

#include "../Panel.h"



class ErosionPanel : public Panel
{
public:
    ErosionPanel(AudioPluginAudioProcessor &p) : Panel(p, "EROSION"),
        erosionAmt(p, "AMOUNT", ParamIDs::erosionAmount),
        erosionFreq(p, "FREQ", ParamIDs::noiseFrequency, ParamUnits::hz),
        erosionQ(p, "Q", ParamIDs::noiseQ)
    {
        
        addAndMakeVisible(erosionAmt);
        addAndMakeVisible(erosionFreq);
        addAndMakeVisible(erosionQ);

        
    }

    void resized() {
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