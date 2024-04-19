#pragma once

#include "juce_core/juce_core.h"
#include "juce_dsp/juce_dsp.h"

template <typename MonoProcessorType, typename StateType>
struct ProcessDuplicator
{
    ProcessDuplicator() : state (new StateType()) {}
    ProcessDuplicator (StateType* stateToUse) : state (stateToUse) {}
    ProcessDuplicator (typename StateType::Ptr stateToUse) : state (std::move (stateToUse)) {}
    ProcessDuplicator (const ProcessDuplicator&) = default;
    ProcessDuplicator (ProcessDuplicator&&) = default;

    void prepare (const ProcessSpec& spec)
    {
        processors.removeRange ((int) spec.numChannels, processors.size());

        while (static_cast<size_t> (processors.size()) < spec.numChannels)
            processors.add (new MonoProcessorType (state));

        auto monoSpec = spec;
        monoSpec.numChannels = 1;

        for (auto* p : processors)
            p->prepare (monoSpec);
    }

    void reset() noexcept      { for (auto* p : processors) p->reset(); }

    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        jassert ((int) context.getInputBlock().getNumChannels()  <= processors.size());
        jassert ((int) context.getOutputBlock().getNumChannels() <= processors.size());

        auto numChannels = static_cast<size_t> (jmin (context.getInputBlock().getNumChannels(),
                                                      context.getOutputBlock().getNumChannels()));

        for (size_t chan = 0; chan < numChannels; ++chan)
            processors[(int) chan]->process (MonoProcessContext<ProcessContext> (context, chan));
    }

    template <typename ProcessContext>
    float processSample (float in, size_t channel, size_t sample) noexcept
    {
        jassert (channel < processors.size());
        return processors[(int) channel]->processSample (in, sample);
    }

    typename StateType::Ptr state;

private:
    template <typename ProcessContext>
    struct MonoProcessContext : public ProcessContext
    {
        MonoProcessContext (const ProcessContext& multiChannelContext, size_t channelToUse)
            : ProcessContext (multiChannelContext), channel (channelToUse)
        {}

        size_t channel;

        typename ProcessContext::ConstAudioBlockType getInputBlock()  const noexcept       { return ProcessContext::getInputBlock() .getSingleChannelBlock (channel); }
        typename ProcessContext::AudioBlockType      getOutputBlock() const noexcept       { return ProcessContext::getOutputBlock().getSingleChannelBlock (channel); }
    };

    juce::OwnedArray<MonoProcessorType> processors;
};

