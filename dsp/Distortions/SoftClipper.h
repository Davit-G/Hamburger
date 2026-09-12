#pragma once

#include "../WaveShapers.h"
#include "../SmoothParam.h"
#include "../../utils/Params.h"
#include "../EffectBase.h"

#include "juce_dsp/juce_dsp.h"
#include "juce_audio_processors/juce_audio_processors.h"

#if PERFETTO
#include <melatonin_perfetto/melatonin_perfetto.h>
#endif // PERFETTO

class SoftClip : public MacroEffect
{
public:
    SoftClip(juce::AudioProcessorValueTreeState& treeState, SlotId slot);

    ~SoftClip();

    void processBlock(juce::dsp::AudioBlock<float>& block) noexcept override;
    void prepare(juce::dsp::ProcessSpec& spec) noexcept override;
private:
    SmoothParam saturationKnob;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoftClip)
};