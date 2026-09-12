#pragma once

#include "juce_core/juce_core.h"
#include "juce_dsp/juce_dsp.h"
#include "juce_audio_processors/juce_audio_processors.h"
#include "../SmoothParam.h"
#include "../../utils/Params.h"
#include "../EffectBase.h"

#if PERFETTO
#include <melatonin_perfetto/melatonin_perfetto.h>
#endif // PERFETTO

//==============================================================================
/*
 */
class Cooked : public MacroEffect
{
public:
    Cooked(juce::AudioProcessorValueTreeState& treeState, SlotId slot);
    ~Cooked();

    void processBlock(juce::dsp::AudioBlock<float> &block) noexcept override;
    void prepare(juce::dsp::ProcessSpec& spec) noexcept override;

private:
    // SmoothParam stages;
    SmoothParam amount;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Cooked)
};
