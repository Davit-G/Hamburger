#pragma once

#include "../../utils/Params.h"

#include "juce_core/juce_core.h"
#include "juce_dsp/juce_dsp.h"
#include "juce_audio_processors/juce_audio_processors.h"

#if PERFETTO
#include <melatonin_perfetto/melatonin_perfetto.h>
#endif // PERFETTO

#include "../SmoothParam.h"
#include "../EffectBase.h"
#include "../EffectInfos.h"
//==============================================================================
/*
 */
class Erosion : public MacroEffect
{
public:
    Erosion(juce::AudioProcessorValueTreeState& treeState)
        : MacroEffect(treeState, SlotId{ModuleId::module1, 0}),
        delayLine(200),
        erosionAmount(getParam(ParamIDs::erosionAmount)),
        erosionFrequency(getParam(ParamIDs::erosionFrequency)),
        erosionQ(getParam(ParamIDs::erosionQ)) {}
    
    ~Erosion() {}

    void processBlock(juce::dsp::AudioBlock<float>& block) override;
    void prepare(juce::dsp::ProcessSpec& spec) override;

private:
    SmoothParam erosionAmount;
    SmoothParam erosionFrequency;
    SmoothParam erosionQ;

    // stereo delay line
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine;

    // iir filter
    // juce::dsp::IIR::Filter<float> iirFilter;
    juce::dsp::IIR::Filter<float> iirFilter;

    // noise generator
    juce::Random random;

    float oldSampleRate;
    float oldSamplesPerBlock;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Erosion)
};