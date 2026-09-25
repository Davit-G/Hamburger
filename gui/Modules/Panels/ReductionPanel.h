#pragma once

 

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"



class ReductionPanel : public Panel
{
public:
    ReductionPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::module1, 0}) : Panel(p, "BIT", &Theme::bitReduction),
                                                   downSample(p, "RATE", slot, ParamIDs::downsampleFreq, ScopeContextType::NOISE),
                                                   bitReduction(p, "BITS", slot, ParamIDs::bitReduction, ScopeContextType::NOISE),
                                                   downsampleMix(p, "MIX", slot, ParamIDs::downsampleMix, ScopeContextType::NOISE)
    {
        addAndMakeVisible(downSample);
        addAndMakeVisible(bitReduction);
        addAndMakeVisible(downsampleMix);
    }

    void resized() override {
        auto bounds = getLocalBounds();
        auto width = bounds.getWidth() / 3;
        downSample.setBounds(bounds.removeFromLeft(width));
        bitReduction.setBounds(bounds.removeFromLeft(width));
        downsampleMix.setBounds(bounds);
    }

    ParamKnob downSample;
    ParamKnob downsampleMix;
    ParamKnob bitReduction;
};