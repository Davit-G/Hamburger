#pragma once

 

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"



class ReductionPanel : public Panel
{
public:
    ReductionPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::module1, 0}) : Panel(p, "BIT", &Theme::bitReduction),
                                                   downSample(p, "RATE", slot, ParamIDs::downsampleFreq, ScopeContextType::NOISE),
                                                   bitReduction(p, "BITS", slot, ParamIDs::bitReduction, ScopeContextType::NOISE)
    {
        addAndMakeVisible(downSample);
        addAndMakeVisible(bitReduction);
    }

    void resized() override {
        auto bounds = getLocalBounds();
        downSample.setBounds(bounds.removeFromLeft(bounds.getWidth() / 2));
        bitReduction.setBounds(bounds);
    }

    ParamKnob downSample;
    ParamKnob bitReduction;
};