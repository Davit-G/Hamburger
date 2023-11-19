#pragma once

#include <JuceHeader.h>

template <typename T>
static T* toBasePointer (dsp::SIMDRegister<T>* r) noexcept
{
    return reinterpret_cast<T*> (r);
}
 
constexpr auto registerSize = dsp::SIMDRegister<float>::size();


class AllPassChain
{
public:
    AllPassChain(juce::AudioProcessorValueTreeState& treeState) : 
        allPassFrequency(treeState, "allPassFreq"), 
        allPassQ(treeState, "allPassQ"), 
        allPassAmount(treeState, "allPassAmount") {}
    ~AllPassChain(){

    }

    template <typename SampleType>
    auto prepareChannelPointers (const dsp::AudioBlock<SampleType>& block)
    {
        std::array<SampleType*, registerSize> result {};

        for (size_t ch = 0; ch < result.size(); ++ch)
            result[ch] = (ch < block.getNumChannels() ? block.getChannelPointer (ch) : zero.getChannelPointer (ch));

        return result;
    }

    void processBlock(dsp::AudioBlock<float> &block)
    {
        TRACE_EVENT("dsp", "AllPassChain::processBlock");
        allPassAmount.update();
        allPassFrequency.update();
        allPassQ.update();

        float amount = allPassAmount.getRaw();
        float freq = allPassFrequency.getRaw();
        float q = allPassQ.getRaw();

        auto context = dsp::ProcessContextReplacing<float>(block);

        const auto& input  = context.getInputBlock();
        const auto numSamples = (int) block.getNumSamples();
        
        auto inChannels = prepareChannelPointers (input);
        if (!((oldAllPassFreq == freq) && (oldAllPassQ == q) && (oldAllPassAmount == amount))) {
            iirCoefficients = dsp::IIR::Coefficients<float>::makeAllPass(oldSampleRate, freq, q);
            for (int i = 0; i < amount; i++) {
                iir[i]->coefficients = iirCoefficients;
            }
            oldAllPassFreq = freq;
            oldAllPassQ = q;
            oldAllPassAmount = amount;
        }

        using Format = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;

        AudioData::interleaveSamples(AudioData::NonInterleavedSource<Format> { inChannels.data(),                                 registerSize, },
                                    AudioData::InterleavedDest<Format>      { toBasePointer (interleaved.getChannelPointer (0)), registerSize },
                                    numSamples);

        for (int i = 0; i < amount; i++) {
            iir[i]->process (dsp::ProcessContextReplacing<dsp::SIMDRegister<float>> (interleaved));
        }

        auto outChannels = prepareChannelPointers (context.getOutputBlock());

        AudioData::deinterleaveSamples(AudioData::InterleavedSource<Format>  { toBasePointer (interleaved.getChannelPointer (0)), registerSize },
                                        AudioData::NonInterleavedDest<Format> { outChannels.data(),                                registerSize },
                                        numSamples);
    }

    void prepare(dsp::ProcessSpec& spec)
    {
        allPassFrequency.prepare(spec);
        allPassQ.prepare(spec);
        allPassAmount.prepare(spec);

        // new SIMD stuff
        iirCoefficients = dsp::IIR::Coefficients<float>::makeAllPass(spec.sampleRate, allPassFrequency.getRaw(), allPassQ.getRaw());
        for (int i = 0; i < 50; i++) {
            iir[i].reset(new dsp::IIR::Filter<dsp::SIMDRegister<float>>(iirCoefficients));
        }

        interleaved = dsp::AudioBlock<dsp::SIMDRegister<float>>(interleavedBlockData, 1, spec.maximumBlockSize);
        zero = dsp::AudioBlock<float>(zeroData, dsp::SIMDRegister<float>::size(), spec.maximumBlockSize);

        zero.clear();

        auto monoSpec = spec;
        monoSpec.numChannels = 1;
        
        for (int i = 0; i < 50; i++) {
            iir[i]->prepare(monoSpec);
        }

        oldSampleRate = spec.sampleRate;
    }

private:
    SmoothParam allPassFrequency;
    SmoothParam allPassQ;
    SmoothParam allPassAmount;

    float oldAllPassFreq = 0.0;
    float oldAllPassQ = 0.0;
    float oldAllPassAmount = 0.0;

    // dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>> allPassFilters[50];

    float oldSampleRate;


    // new SIMD stuff

    dsp::IIR::Coefficients<float>::Ptr iirCoefficients;                 // [1]
    std::unique_ptr<dsp::IIR::Filter<dsp::SIMDRegister<float>>> iir[50];

    dsp::AudioBlock<dsp::SIMDRegister<float>> interleaved;
    dsp::AudioBlock<float> zero;

    HeapBlock<char> interleavedBlockData, zeroData;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AllPassChain)
};