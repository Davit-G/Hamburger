#pragma once

#include "juce_core/juce_core.h"
#include "juce_dsp/juce_dsp.h"
#include "juce_audio_processors/juce_audio_processors.h"
#include "../SmoothParam.h"
#include "../../utils/Params.h"

#include <melatonin_perfetto/melatonin_perfetto.h>

//==============================================================================
/*
 */
class Cooked
{
public:
    Cooked(juce::AudioProcessorValueTreeState& treeState);
    ~Cooked();

    void processBlock(juce::dsp::AudioBlock<float> &block) noexcept;
    void prepare(juce::dsp::ProcessSpec& spec) noexcept;

private:
    SmoothParam amount;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Cooked)
};
