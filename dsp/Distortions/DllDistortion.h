#pragma once

#include "../SmoothParam.h"
#include "../EffectBase.h"
#include "../EffectInfos.h"
#include "../../hotdsp/HotDsp.h"

class DllDistortion : public MacroEffect, private juce::Timer
{
public:
    DllDistortion (juce::AudioProcessorValueTreeState& state, SlotId slotId)
        : MacroEffect (state, slotId), treeState (state), pathKey (pathKeyFor (slotId)), statusKey (statusKeyFor (slotId))
    {
        for (const auto& info : EffectInfos::dll.params)
        {
            if (info.id.isNull())
                continue;

            params.emplace_back (getParam (info));
            names.push_back (info.displayName.toStdString());
            paramInfos.push_back ({ nullptr, info.range.start, info.range.end, info.defaultValue });
        }

        // names is done growing, so what they point to stays put
        for (size_t p = 0; p < names.size(); ++p)
            paramInfos[p].name = names[p].c_str();

        startTimer (1000);
    }

    ~DllDistortion() override { stopTimer(); }

    // where the DLL box keeps the dylib's path, and where this says how loading it went. on the state, so a session reopens it
    static juce::Identifier pathKeyFor (SlotId slot) { return slot.type().getParamID() + "_dllPath"; }
    static juce::Identifier statusKeyFor (SlotId slot) { return slot.type().getParamID() + "_dllStatus"; }

    void prepare (juce::dsp::ProcessSpec& spec) override
    {
        for (auto& param : params)
            param.prepare (spec);

        paramBuffers.assign (params.size(), std::vector<float> (spec.maximumBlockSize));
        paramPointers.clear();

        for (auto& buffer : paramBuffers)
            paramPointers.push_back (buffer.data());

        channelPointers.resize (spec.numChannels);

        const juce::SpinLock::ScopedLockType lock (swapLock);
        hotSpec = { spec.sampleRate, (int) spec.numChannels, (int) spec.maximumBlockSize, paramInfos.data(), (int) paramInfos.size() };

        if (loaded != nullptr)
            loaded->dsp->prepare (hotSpec);
    }

    void processBlock (juce::dsp::AudioBlock<float>& block) override
    {
        const juce::SpinLock::ScopedTryLockType lock (swapLock);

        if (! lock.isLocked() || loaded == nullptr || hotSpec.maxSamples <= 0)
            return;

        for (auto& param : params)
            param.update();

        const auto numChannels = juce::jmin ((int) block.getNumChannels(), (int) channelPointers.size());
        const auto total = (int) block.getNumSamples();

        // the declicking pass hands over more than the block size, so it goes over in pieces
        for (int start = 0; start < total; start += hotSpec.maxSamples)
        {
            const auto numSamples = juce::jmin (hotSpec.maxSamples, total - start);

            for (int ch = 0; ch < numChannels; ++ch)
                channelPointers[(size_t) ch] = block.getChannelPointer ((size_t) ch) + start;

            // channel 0's smoothing and modulation for every channel, change to per channel params if stereo mod matters
            for (size_t p = 0; p < params.size(); ++p)
                for (int n = 0; n < numSamples; ++n)
                    paramBuffers[p][(size_t) n] = params[p].getNextValue (0);

            loaded->dsp->process (channelPointers.data(), numChannels, numSamples, paramPointers.data(), (int) params.size());
        }
    }

private:
    struct Loaded
    {
        juce::File copy;
        juce::DynamicLibrary library;
        std::unique_ptr<HotDsp> dsp;

        ~Loaded()
        {
            dsp.reset();
            library.close();
            copy.deleteFile();
        }
    };

    void timerCallback() override
    {
        const juce::File file (treeState.state[pathKey].toString());
        const auto stamp = file.getFullPathName() + "@" + juce::String (file.getLastModificationTime().toMilliseconds());

        if (stamp == loadedStamp)
            return;

        // a build still writing it keeps changing it, so it waits for a tick where it stayed the same
        if (stamp != seenStamp)
        {
            seenStamp = stamp;
            return;
        }

        loadedStamp = stamp;
        load (file);
    }

    void load (const juce::File& file)
    {
        std::unique_ptr<Loaded> next;

        if (file.existsAsFile())
        {
            next = std::make_unique<Loaded>();

            // a fresh copy each time: dlopen hands back what it already has for a path it's seen, and macOS might never
            // actually unload a dylib, so a new path is the only way to be sure it's the new code
            next->copy = juce::File::createTempFile (".dylib");

            if (file.copyFileTo (next->copy) && next->library.open (next->copy.getFullPathName()))
                if (auto create = (HotDspCreate) next->library.getFunction ("hotdspCreate"))
                    next->dsp.reset (create());

            if (next->dsp == nullptr)
            {
                DBG ("DLL distortion: couldn't load " + file.getFullPathName());
                treeState.state.setProperty (statusKey, "couldn't load " + file.getFileName(), nullptr);
                return; // the one already running keeps going
            }
        }

        {
            const juce::SpinLock::ScopedLockType lock (swapLock);

            if (next != nullptr && hotSpec.maxSamples > 0)
                next->dsp->prepare (hotSpec);

            std::swap (loaded, next);
        }

        // next is the old one now, destroyed here on the message thread
        next.reset();

        treeState.state.setProperty (statusKey, file.existsAsFile() ? "loaded " + juce::Time::getCurrentTime().toString (false, true, true, true)
                                                                    : juce::String ("nothing loaded"), nullptr);
    }

    juce::AudioProcessorValueTreeState& treeState;
    const juce::Identifier pathKey, statusKey;
    juce::String seenStamp, loadedStamp;

    std::vector<SmoothParam> params;
    std::vector<std::string> names;
    std::vector<HotDspParam> paramInfos;

    std::vector<std::vector<float>> paramBuffers;
    std::vector<const float*> paramPointers;
    std::vector<float*> channelPointers;

    juce::SpinLock swapLock;
    HotDspSpec hotSpec {};
    std::unique_ptr<Loaded> loaded;
};
