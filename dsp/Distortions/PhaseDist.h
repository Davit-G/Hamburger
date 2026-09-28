#pragma once

#include "../SmoothParam.h"
#include "../WaveShapers.h"
#include "../../utils/Params.h"
#include "../FrequencyShifting/HilbertBiquad.h"

#include "../EffectBase.h"

inline float weirdRectify(float x, float a) {
	return a * powf(std::abs(x) * 1.5f, 2.0f) + x * (1.0f - a);
}

#if PERFETTO
#include <melatonin_perfetto/melatonin_perfetto.h>
#endif // PERFETTO
//==============================================================================
/*
 */
class PhaseDist : public MacroEffect
{
public:
    PhaseDist(juce::AudioProcessorValueTreeState& treeState, SlotId slot);
    ~PhaseDist() {}

    void processBlock(juce::dsp::AudioBlock<float> &block) noexcept override;
    void prepare(juce::dsp::ProcessSpec& spec) noexcept override;

private:
    SmoothParam amount;
    SmoothParam tone;
    SmoothParam stereo;
    SmoothParam rectify;
    SmoothParam shift;

    HilbertBiquadShifter hilbertTransformL;
    HilbertBiquadShifter hilbertTransformR;

    // under a downward shift, whatever's below it would fold back off 0 Hz, so it's taken out first
    static constexpr double minFoldbackHz = 5.0;
    ChebyshevHighpass foldbackL, foldbackR;
    double foldbackCutoff = 0.0;

    float sampleRate;
    float sampleRateMult;

    // delay line for phase distortion effect
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine;

    // filter to remove harsh freqs from phase distorted signal
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> filter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhaseDist)
};
