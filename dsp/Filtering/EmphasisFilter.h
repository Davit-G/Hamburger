#pragma once

#include "juce_core/juce_core.h"
#include "chowdsp_filters/chowdsp_filters.h"

#include <complex>

#include "../EffectBase.h"
#include "../SmoothParam.h"

class EmphasisFilter
{
public:
    // keep in line with ParamIDs::emphasisTypes
    enum Mode { bands, tilt };

    /*  The tilt: a low shelf down by half of it and a high shelf up by half, both on the middle of the audible range, with
        a Q so low their slopes run into one - within a dB of a straight line from 20 Hz to 20 kHz. */
    static constexpr float tiltPivotHz = 632.0f, tiltQ = 0.2f;

    // what the tilt does at a frequency in dB, from the shelves' analog prototypes (chowdsp's SVF shelves), for drawing it
    static double tiltResponseDb(double freq, double tiltDb)
    {
        const auto shelf = [freq](double db, bool high)
        {
            const auto a = std::pow(10.0, db / 40.0);
            const auto k = 1.0 / (double) tiltQ;
            const auto s = std::complex<double>(0.0, freq / ((double) tiltPivotHz * (high ? std::sqrt(a) : 1.0 / std::sqrt(a))));
            const auto h = high ? (a * a * s * s + k * a * s + 1.0) / (s * s + k * s + 1.0)
                                : (s * s + k * a * s + a * a) / (s * s + k * s + 1.0);
            return 20.0 * std::log10(std::abs(h));
        };

        return shelf(-tiltDb * 0.5, false) + shelf(tiltDb * 0.5, true);
    }

    explicit EmphasisFilter(juce::AudioProcessorValueTreeState& treeState)
        : tiltSmooth(treeState, ParamIDs::emphasisTilt),
          emphasisLowSmooth(treeState, ParamIDs::emphasisLowGain),
          emphasisHighSmooth(treeState, ParamIDs::emphasisHighGain),
          emphasisLowFreqSmooth(treeState, ParamIDs::emphasisLowFreq),
          emphasisHighFreqSmooth(treeState, ParamIDs::emphasisHighFreq)
    {
        enableEmphasis = dynamic_cast<juce::AudioParameterBool *>(treeState.getParameter(ParamIDs::emphasisOn.getParamID()));
        jassert(enableEmphasis);

        type = dynamic_cast<juce::AudioParameterChoice *>(treeState.getParameter(ParamIDs::emphasisType.getParamID()));
        jassert(type);
    }

    void prepare(juce::dsp::ProcessSpec& spec)
    {
        for (int channel = 0; channel < 2; ++channel)
        {
            for (int i = 0; i < 2; ++i)
            {
                peakFilterBeforeSVF[i].prepare(spec);
                peakFilterBeforeSVF[1].setQValue(0.5f);
                peakFilterAfterSVF[i].prepare(spec);
                peakFilterAfterSVF[1].setQValue(0.5f);
            }
        }

        for (auto* shelves : { &tiltBefore, &tiltAfter })
            shelves->prepare(spec);

        tiltSmooth.prepare(spec);
        emphasisLowSmooth.prepare(spec);
        emphasisHighSmooth.prepare(spec);
        emphasisLowFreqSmooth.prepare(spec);
        emphasisHighFreqSmooth.prepare(spec);

        sampleRate = spec.sampleRate;

        const auto blockSize = static_cast<size_t>(spec.maximumBlockSize);
        emphasisLowBuffer.resize(blockSize, 0.0f);
        emphasisHighBuffer.resize(blockSize, 0.0f);
        emphasisLowFreqBuffer.resize(blockSize, 0.0f);
        emphasisHighFreqBuffer.resize(blockSize, 0.0f);
        tiltBuffer.resize(blockSize, 0.0f);
    }

    void beforeProcessing(size_t numSamples) {
        if (enableEmphasis != nullptr)
            emphasisOn = enableEmphasis->get();

        mode = type != nullptr ? type->getIndex() : bands;

        tiltSmooth.update();
        emphasisLowSmooth.update();
        emphasisHighSmooth.update();
        emphasisLowFreqSmooth.update();
        emphasisHighFreqSmooth.update();

        if (!emphasisOn) return;

        if (mode == tilt)
        {
            tiltNeedsUpdates = tiltSmooth.isSmoothing(0);

            if (tiltNeedsUpdates)
                for (size_t sample = 0; sample < numSamples; ++sample)
                    tiltBuffer[sample] = tiltSmooth.getNextValue(0);

            return;
        }

        parametersNeedUpdates = emphasisLowSmooth.isSmoothing(0) || emphasisHighSmooth.isSmoothing(0) || emphasisLowFreqSmooth.isSmoothing(0) || emphasisHighFreqSmooth.isSmoothing(0);
        
        if (parametersNeedUpdates) {
            for (size_t sample = 0; sample < numSamples; ++sample)
            {
                emphasisLowBuffer[sample] = emphasisLowSmooth.getNextValue(0);
                emphasisHighBuffer[sample] = emphasisHighSmooth.getNextValue(0);
                emphasisLowFreqBuffer[sample] = emphasisLowFreqSmooth.getNextValue(0);
                emphasisHighFreqBuffer[sample] = emphasisHighFreqSmooth.getNextValue(0);
            }
        }
            
    }

    void processBefore(juce::dsp::AudioBlock<float>& block)
    {
        if (!emphasisOn)
            return;

        if (mode == tilt)
            return tiltBefore.process(block, *this, -1.0f);

        const auto numChannels = block.getNumChannels();
        const auto numSamples = block.getNumSamples();

        for (size_t sample = 0; sample < numSamples; ++sample)
        {
            if (parametersNeedUpdates && (sample % samplesToSkip == 0)) {
                const auto nextEmphasisLow = -emphasisLowBuffer[sample];
                const auto nextEmphasisHigh = -emphasisHighBuffer[sample];
                const auto nextEmphasisLowFreq = emphasisLowFreqBuffer[sample];
                const auto nextEmphasisHighFreq = emphasisHighFreqBuffer[sample];

                peakFilterBeforeSVF[0].setCutoffFrequency(nextEmphasisLowFreq);
                peakFilterBeforeSVF[0].setGainDecibels(nextEmphasisLow);
                peakFilterBeforeSVF[1].setCutoffFrequency(nextEmphasisHighFreq);
                peakFilterBeforeSVF[1].setGainDecibels(nextEmphasisHigh);
            }

            for (size_t channel = 0; channel < numChannels; ++channel)
            {
                const auto input = block.getSample(channel, sample);
                const auto interm = peakFilterBeforeSVF[0].processSample(channel, input);
                block.setSample(channel, sample, peakFilterBeforeSVF[1].processSample(channel, interm));
            }
        }

        peakFilterBeforeSVF[0].snapToZero();
        peakFilterBeforeSVF[1].snapToZero();
    }

    void processAfter(juce::dsp::AudioBlock<float>& block)
    {
        if (!emphasisOn)
            return;

        if (mode == tilt)
            return tiltAfter.process(block, *this, 1.0f);

        const auto numChannels = block.getNumChannels();
        const auto numSamples = block.getNumSamples();
        // follows the same smoothed gains as the bells, a jump straight to the knob's target each block zippers
        const auto compensationFor = [](float lowDb, float highDb) { return juce::Decibels::decibelsToGain(-(lowDb + highDb) * 0.133f); };
        auto eqCompensation = compensationFor(emphasisLowSmooth.getRaw(0), emphasisHighSmooth.getRaw(0));

        for (size_t sample = 0; sample < numSamples; ++sample)
        {
            if (parametersNeedUpdates)
                eqCompensation = compensationFor(emphasisLowBuffer[sample], emphasisHighBuffer[sample]);

            if (parametersNeedUpdates && (sample % samplesToSkip == 0)) {
                const auto nextEmphasisLow = emphasisLowBuffer[sample];
                const auto nextEmphasisHigh = emphasisHighBuffer[sample];
                const auto nextEmphasisLowFreq = emphasisLowFreqBuffer[sample];
                const auto nextEmphasisHighFreq = emphasisHighFreqBuffer[sample];

                peakFilterAfterSVF[0].setCutoffFrequency(nextEmphasisLowFreq);
                peakFilterAfterSVF[0].setGainDecibels(nextEmphasisLow);
                peakFilterAfterSVF[1].setCutoffFrequency(nextEmphasisHighFreq);
                peakFilterAfterSVF[1].setGainDecibels(nextEmphasisHigh);
            }

            for (size_t channel = 0; channel < numChannels; ++channel)
            {
                const auto input = block.getSample(channel, sample);
                const auto interm = peakFilterAfterSVF[0].processSample(channel, input);
                block.setSample(channel, sample, peakFilterAfterSVF[1].processSample(channel, interm) * eqCompensation);
            }
        }

        peakFilterAfterSVF[0].snapToZero();
        peakFilterAfterSVF[1].snapToZero();
    }

private:
    // one side's tilt: the knob's way round after the distortion, the other way before it
    struct TiltShelves
    {
        chowdsp::SVFLowShelf<float> low;
        chowdsp::SVFHighShelf<float> high;

        void prepare(juce::dsp::ProcessSpec& spec)
        {
            low.prepare(spec);
            high.prepare(spec);
            low.setCutoffFrequency(tiltPivotHz);
            high.setCutoffFrequency(tiltPivotHz);
            low.setQValue(tiltQ);
            high.setQValue(tiltQ);
            set(0.0f);
        }

        void set(float tiltDb)
        {
            low.setGainDecibels(-tiltDb * 0.5f);
            high.setGainDecibels(tiltDb * 0.5f);
        }

        void process(juce::dsp::AudioBlock<float>& block, EmphasisFilter& owner, float direction)
        {
            const auto numChannels = block.getNumChannels();

            if (!owner.tiltNeedsUpdates)
                set(owner.tiltSmooth.getRaw(0) * direction);

            for (size_t sample = 0; sample < block.getNumSamples(); ++sample)
            {
                if (owner.tiltNeedsUpdates && sample % (size_t) owner.samplesToSkip == 0)
                    set(owner.tiltBuffer[sample] * direction);

                for (size_t channel = 0; channel < numChannels; ++channel)
                    block.setSample((int) channel, (int) sample, high.processSample((int) channel, low.processSample((int) channel, block.getSample((int) channel, (int) sample))));
            }

            low.snapToZero();
            high.snapToZero();
        }
    };

    double sampleRate = 44100.0;
    bool emphasisOn = true;
    int mode = bands;

    juce::AudioParameterChoice* type = nullptr;
    SmoothParam tiltSmooth;
    std::vector<float> tiltBuffer {};
    bool tiltNeedsUpdates = true;
    TiltShelves tiltBefore, tiltAfter;

    bool parametersNeedUpdates = true;

    SmoothParam emphasisLowSmooth;
    SmoothParam emphasisHighSmooth;
    SmoothParam emphasisLowFreqSmooth;
    SmoothParam emphasisHighFreqSmooth;

    std::vector<float> emphasisLowBuffer {};
    std::vector<float> emphasisHighBuffer {};
    std::vector<float> emphasisLowFreqBuffer {};
    std::vector<float> emphasisHighFreqBuffer {};

    juce::AudioParameterBool* enableEmphasis = nullptr;

    int samplesToSkip = 8;

    chowdsp::SVFBell<float> peakFilterBeforeSVF[2];
    chowdsp::SVFBell<float> peakFilterAfterSVF[2];
};

// same instance applies to either pre or post
class EmphasisEffectFilter : public EffectBase {
public:
    EmphasisEffectFilter(EmphasisFilter& eRef, bool post) : emphasis(eRef) {
        isPost = post;
    }

    ~EmphasisEffectFilter() {}

    void prepare(juce::dsp::ProcessSpec& spec) override {};
    
    void processBlock(juce::dsp::AudioBlock<float>& block) override {
        switch (isPost) {
            case false: {
                emphasis.processBefore(block);
                break;
            }
            case true: {
                emphasis.processAfter(block);
                break;
            }
        }
    }

private:
    EmphasisFilter& emphasis;

    bool isPost = false;
};