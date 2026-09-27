#pragma once

#include "EffectBase.h"
#include "EffectInfos.h"
#include "Crossover.h"
#include "LinearPhaseSplit.h"
#include "DCBlockingHighPass.h"
#include "PrimaryDistortion.h"
#include "SmoothParam.h"
#include "FrequencyShifting/HilbertBiquad.h"
#include "../gui/Modules/ScopeDataCollector.h"

//  Owns the main stage's distortion slots and decides how audio is routed through them
class MainRouting : public EffectBase,
                    private juce::ValueTree::Listener
{
public:
    /*  The band splits' slope, 12, 24 or 48 dB/oct, or linear phase for every crossover in the plugin. With the linear
        highpass, which swaps every DC blocker in the distortion chain for one that leaves the phase alone, and whether dry
        signals are put through the crossovers' allpasses to mix back in phase. Kept in the plugin state rather than as
        parameters, so they save with presets and projects but aren't automatable. */
    static inline const juce::Identifier crossoverSlopeProperty { "crossoverSlope" };
    static inline const juce::Identifier linearHighpassProperty { "linearHighpass" };
    static inline const juce::Identifier dryPhaseProperty { "dryPhaseCompensation" };
    static constexpr int linearPhase = 1; // stored in place of a slope

    static int crossoverSlopeFrom (const juce::var& stored)
    {
        const auto slope = (int) stored;
        return slope == 12 || slope == 48 || slope == linearPhase ? slope : 24;
    }

    bool linearCrossovers() const { return crossoverSlope.load (std::memory_order_relaxed) == linearPhase; }
    bool linearHighpasses() const { return linearHighpass.load (std::memory_order_relaxed); }
    bool compensatesDryPhase() const { return dryPhase.load (std::memory_order_relaxed); }

    // keep in line with ParamIDs::routingTypes. multiband's band count is a parameter of its own, ParamIDs::bandCount
    enum Routing { stack, multiband, midSide, exciter };

    // keep in line with ParamIDs::stackFilterTypes
    enum StackFilter { noFilter, allpassFilter, hilbertFilter, bandpassFilter, notchFilter };

    static constexpr int maxSlots = 4;
    static_assert (maxSlots == ParamIDs::numBands);
    
    struct BandLayout
    {
        int numBands = 0;
        std::array<int, maxSlots> slots {};
        std::array<int, maxSlots - 1> crossovers {};
    };

    static BandLayout multibandLayout (int bandCount)
    {
        switch (bandCount)
        {
            case 3:  return { 3, { 0, 2, 3 },    { 0, 2 } };
            case 4:  return { 4, { 0, 1, 2, 3 }, { 0, 1, 2 } };
            default: return { 2, { 0, 3 },       { 0 } };
        }
    }

    // the multiband routing's layout at its band count, the exciter's two paths, or nothing for the rest
    static BandLayout bandLayout (int routingIndex, int bandCount)
    {
        switch (routingIndex)
        {
            case multiband: return multibandLayout (bandCount);
            case exciter:   return { 2, { 0, 3 }, { 0 } };
            default:        return {};
        }
    }

    // whether a routing runs a slot at all, the rest sit idle
    static bool usesSlot (int routingIndex, int bandCount, int slot)
    {
        switch (routingIndex)
        {
            case stack:   return slot == 0;
            case midSide: return slot == 0 || slot == 1;
            default: break;
        }

        const auto layout = bandLayout (routingIndex, bandCount);

        for (int band = 0; band < layout.numBands; ++band)
            if (layout.slots[(size_t) band] == slot)
                return true;

        return false;
    }

    MainRouting (juce::AudioProcessorValueTreeState& state, ScopeDataCollector<float>& scopeCollector)
        : crossoverLow (state, ParamIDs::crossoverLow),
          crossoverMid (state, ParamIDs::crossoverMid),
          crossoverHigh (state, ParamIDs::crossoverHigh),
          stackGain (state, ParamIDs::stackGain),
          stackMix (state, ParamIDs::stackMix),
          stackFilterFreq (state, ParamIDs::stackFilterFreq),
          stackFilterQ (state, ParamIDs::stackFilterQ),
          stackRotation (state, ParamIDs::stackRotation),
          msBalance (state, ParamIDs::msBalance),
          apvts (state),
          scope (scopeCollector)
    {
        /*  The only waveshape curve shared anywhere: slot 1's, with the stack's extra stages below, which are copies of slot 1
            on its own settings and only ever run in the stack. Slots 2 to 4 get none passed, so each builds its own, and
            pre and post distortion are separate processors with their own too. In every routing but the stack - the band
            split, mid/side, the exciter - no two slots share anything. */
        const auto slot1Curve = std::make_shared<Waveshape::SharedCurve>();

        for (int i = 0; i < maxSlots; ++i)
            slots[(size_t) i] = std::make_unique<PrimaryDistortion> (state, SlotId { ModuleId::main, i }, i == 0 ? slot1Curve : nullptr);

        /*  Stack runs slot 0's settings several times over, but it cannot reuse slot 0's object to
            do it: a processor with memory would start each pass holding state from the end of the
            previous one, which is a discontinuity once per block. Separate instances on the same
            SlotId give identical parameters with their own state, so every stage sees a
            continuous stream. Stage 0 is slot 0 itself. */
        for (auto& stage : extraStackStages)
            stage = std::make_unique<PrimaryDistortion> (state, SlotId { ModuleId::main, 0 }, slot1Curve);

        routing = dynamic_cast<juce::AudioParameterChoice*> (
            state.getParameter (ParamIDs::mainRouting.getParamID()));
        jassert (routing);

        stackCount = dynamic_cast<juce::AudioParameterInt*> (
            state.getParameter (ParamIDs::stackCount.getParamID()));
        jassert (stackCount);

        bandCount = dynamic_cast<juce::AudioParameterInt*> (
            state.getParameter (ParamIDs::bandCount.getParamID()));
        jassert (bandCount);

        stackFlip = dynamic_cast<juce::AudioParameterBool*> (
            state.getParameter (ParamIDs::stackFlip.getParamID()));
        jassert (stackFlip);

        stackFilter = dynamic_cast<juce::AudioParameterChoice*> (
            state.getParameter (ParamIDs::stackFilter.getParamID()));
        jassert (stackFilter);

        for (int band = 0; band < maxSlots; ++band)
        {
            bandMutes[(size_t) band] = dynamic_cast<juce::AudioParameterBool*> (state.getParameter (ParamIDs::bandMutes[band]->getParamID()));
            bandSolos[(size_t) band] = dynamic_cast<juce::AudioParameterBool*> (state.getParameter (ParamIDs::bandSolos[band]->getParamID()));
            jassert (bandMutes[(size_t) band] && bandSolos[(size_t) band]);
        }

        // the state tree gets swapped whole on a preset or project load, which is valueTreeRedirected
        apvts.state.addListener (this);
        syncSettings();
    }

    ~MainRouting() override { apvts.state.removeListener (this); }

    void prepare (juce::dsp::ProcessSpec& spec) override
    {
        for (auto& slot : slots)
            slot->prepare (spec);

        for (auto& stage : extraStackStages)
            stage->prepare (spec);

        crossoverLow.prepare (spec);
        crossoverMid.prepare (spec);
        crossoverHigh.prepare (spec);
        stackGain.prepare (spec);
        stackMix.prepare (spec);
        stackFilterFreq.prepare (spec);
        stackFilterQ.prepare (spec);
        stackRotation.prepare (spec);
        msBalance.prepare (spec);

        sampleRate = spec.sampleRate;

        stackDryWet.prepare (spec);
        stackDryWet.setWetMixProportion (stackMix.getRaw() * 0.01f);
        stackDryWet.reset();

        for (auto* path : { &wetPath, &dryPath })
        {
            for (auto& allpass : path->allpasses)
            {
                allpass.prepare (spec);
                allpass.setType (juce::dsp::FirstOrderTPTFilterType::allpass);
            }

            for (auto& band : path->bands)
            {
                band.prepare (spec);
                band.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
            }

            for (auto& pair : path->rotators)
                for (auto& rotator : pair)
                    rotator.prepare (spec);

            for (auto& rotation : path->rotations)
                rotation.reset (spec.sampleRate, 0.02);
        }

        for (auto& rotation : wetPath.rotations)
            rotation.setCurrentAndTargetValue (stackRotation.getRaw());

        for (auto& split : splits)
            split.prepare (spec);

        linearSplit.prepare (spec);

        for (auto& row : allpasses)
            for (auto& compensator : row)
                compensator.prepare (spec);

        for (auto& blocker : pathBlockers)
            blocker.prepare (spec);

        for (auto& blocker : dryBlockers)
            blocker.prepare (spec);

        for (int band = 0; band < maxSlots; ++band)
        {
            bandLevels[(size_t) band].reset (spec.sampleRate, 0.02);
            bandLevels[(size_t) band].setCurrentAndTargetValue (bandLevel (band, multibandLayout (4)));
        }

        // one scratch buffer per slot, so a band can be handed to a slot as its own block
        for (auto& buffer : bandBuffers)
            buffer.setSize ((int) spec.numChannels, (int) spec.maximumBlockSize, false, true, true);
    }

    void processBlock (juce::dsp::AudioBlock<float>& block) override
    {
        const auto slope = linearCrossovers() ? 24 : crossoverSlope.load (std::memory_order_relaxed);

        for (auto& split : splits)
            split.setSlope (slope);

        for (auto& row : allpasses)
            for (auto& compensator : row)
                compensator.setSlope (slope);

        const auto linear = linearHighpasses();

        for (auto& slot : slots)
            slot->setLinearHighpass (linear);

        for (auto& stage : extraStackStages)
            stage->setLinearHighpass (linear);

        for (auto* blockers : { &pathBlockers, &dryBlockers })
            for (auto& blocker : *blockers)
                blocker.setLinear (linear);

        crossoverLow.update();
        crossoverMid.update();
        crossoverHigh.update();
        stackGain.update();
        stackMix.update();
        stackFilterFreq.update();
        stackFilterQ.update();
        stackRotation.update();
        msBalance.update();

        switch (currentRouting())
        {
            case multiband: processBands (block, multibandLayout (currentBandCount())); break;
            case midSide:   processMidSide (block);       break;
            case exciter:   processExciter (block);       break;
            case stack:
            default:        processStack (block);         break;
        }

        // the routings without a split are delayed as much as the ones with, so the latency never moves
        const auto split = currentRouting() == multiband || currentRouting() == exciter;

        if (linearCrossovers() && ! split)
            linearSplit.delayBy (block);
    }

    void updateParamsEveryBlock() override
    {
        for (auto& slot : slots)
            slot->updateParamsEveryBlock();
    }

    //  which slot's input and output feed the scope's in/out trace, -1 for none.
    void setScopeTap (int slot, int oversamplingFactor)
    {
        tapSlot = slot;
        tapOversampling = oversamplingFactor;
    }

    int getLatencySamples() const override
    {
        // the band routings run slots in parallel, so their latency is the worst slot, not the sum
        auto total = 0;

        for (const auto& slot : slots)
            total = juce::jmax (total, slot->getLatencySamples());

        return total + (linearCrossovers() ? linearSplit.getLatency() : 0);
    }

    Crossovers getCrossovers() const override
    {
        if (linearCrossovers())
            return {};

        const auto layout = bandLayout (currentRouting(), currentBandCount());
        const float hz[] { crossoverLow.getRaw (0), crossoverMid.getRaw (0), crossoverHigh.getRaw (0) };

        Crossovers crossovers { crossoverSlope.load (std::memory_order_relaxed), juce::jmax (0, layout.numBands - 1) };

        for (int i = 0; i < crossovers.count; ++i)
            crossovers.hz[(size_t) i] = hz[layout.crossovers[(size_t) i]];

        return crossovers;
    }

private:
    int currentRouting() const { return routing != nullptr ? routing->getIndex() : stack; }
    int currentBandCount() const { return bandCount != nullptr ? bandCount->get() : 2; }

    struct StackPath
    {
        std::array<juce::dsp::FirstOrderTPTFilter<float>, maxSlots - 1> allpasses;
        std::array<juce::dsp::StateVariableTPTFilter<float>, maxSlots - 1> bands; // the bandpass, and the notch from it
        std::array<std::array<HilbertBiquadShifter, 2>, maxSlots - 1> rotators;
        std::array<juce::SmoothedValue<float>, maxSlots - 1> rotations; // degrees
    };

    // slot 1 on its own, for a mono block mid/side can't split
    void processFirstSlot (juce::dsp::AudioBlock<float>& block)
    {
        tapIn (0, block);
        slots[0]->processBlock (block);
        tapOut (0, block);
    }

    void processStack (juce::dsp::AudioBlock<float>& block)
    {
        const auto count = stackCount != nullptr ? stackCount->get() : 1;
        const auto flip = stackFlip != nullptr && stackFlip->get();
        const auto gain = juce::Decibels::decibelsToGain (stackGain.getRaw (0)) * (flip ? -1.0f : 1.0f);

        // the stack only ever runs slot 0's settings, so it's traced whole whichever main box is on screen.
        const auto tapping = tapSlot >= 0;

        if (tapping)
            scope.capturePreDistortion (block.getChannelPointer (0), block.getNumSamples(), tapOversampling);

        pushStackDry (block, count);

        for (auto& rotation : wetPath.rotations)
            rotation.setTargetValue (stackRotation.getRaw());

        for (int stage = 0; stage < count; ++stage)
        {
            if (stage > 0)
            {
                block.multiplyBy (gain);
                filterGap (wetPath, stage - 1, block);
            }

            stageFor (stage).processBlock (block);

            // per stage, so each one keeps its own filter state across blocks. with slot 1 off nothing's made any DC to take out
            if (slots[0]->isEnabled())
                pathBlockers[(size_t) juce::jmin (stage, maxSlots - 1)].processBlock (block);
        }

        // an even number of flips leaves the signal inverted against everything else in the chain
        if (flip && count % 2 == 0)
            block.multiplyBy (-1.0f);

        stackDryWet.setWetMixProportion (stackMix.getRaw() * 0.01f);
        stackDryWet.mixWetSamples (block);

        if (tapping)
            scope.capturePostDistortion (block.getChannelPointer (0), block.getNumSamples(), tapOversampling);
    }


    void pushStackDry (const juce::dsp::AudioBlock<float>& block, int count)
    {
        auto dry = juce::dsp::AudioBlock<float> (bandBuffers[0])
                       .getSubBlock (0, block.getNumSamples())
                       .getSubsetChannelBlock (0, block.getNumChannels());

        dry.copyFrom (block);

        for (int stage = 0; stage < count; ++stage)
        {
            if (stage > 0 && ! stackFilterShapesLevel())
                filterGap (dryPath, stage - 1, dry);

            // skipped with the wet's, to stay in step with it
            if (slots[0]->isEnabled())
                dryBlockers[(size_t) juce::jmin (stage, maxSlots - 1)].processBlock (dry);
        }

        stackDryWet.pushDrySamples (dry);
    }

    int currentStackFilter() const { return stackFilter != nullptr ? stackFilter->getIndex() : noFilter; }
    bool stackFilterShapesLevel() const { return currentStackFilter() == bandpassFilter || currentStackFilter() == notchFilter; }

    // the filter going into stage gap + 1
    void filterGap (StackPath& path, int gap, juce::dsp::AudioBlock<float>& block)
    {
        const auto i = (size_t) gap;

        switch (currentStackFilter())
        {
            case allpassFilter:
            {
                path.allpasses[i].setCutoffFrequency (juce::jmin (stackFilterFreq.getRaw(), (float) sampleRate * 0.45f));

                juce::dsp::ProcessContextReplacing<float> context (block);
                path.allpasses[i].process (context);
                break;
            }

            case hilbertFilter: rotate (path.rotators[i], path.rotations[i], block); break;

            case bandpassFilter:
            case notchFilter:
            {
                auto& band = path.bands[i];
                const auto q = stackFilterQ.getRaw();
                const auto notch = currentStackFilter() == notchFilter;

                band.setCutoffFrequency (juce::jmin (stackFilterFreq.getRaw(), (float) sampleRate * 0.45f));
                band.setResonance (q);

                // the SVF's bandpass peaks at q, so over q it's unity at the centre, and the input less that is the notch
                for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
                {
                    auto* samples = block.getChannelPointer (ch);

                    for (size_t n = 0; n < block.getNumSamples(); ++n)
                    {
                        const auto passed = band.processSample ((int) ch, samples[n]) / q;
                        samples[n] = notch ? samples[n] - passed : passed;
                    }
                }
                break;
            }

            default: break;
        }
    }

    static void rotate (std::array<HilbertBiquadShifter, 2>& rotators, juce::SmoothedValue<float>& degrees, juce::dsp::AudioBlock<float>& block)
    {
        const auto numChannels = juce::jmin (block.getNumChannels(), rotators.size());

        for (size_t n = 0; n < block.getNumSamples(); ++n)
        {
            const auto theta = juce::degreesToRadians (degrees.getNextValue());
            const auto c = std::cos (theta), s = std::sin (theta);

            for (size_t ch = 0; ch < numChannels; ++ch)
            {
                auto* samples = block.getChannelPointer (ch);
                samples[n] = rotators[ch].rotate (samples[n], c, s);
            }
        }
    }

    //  split into numBands
    void processBands (juce::dsp::AudioBlock<float>& block, const BandLayout& layout)
    {
        const auto numSamples = (int) block.getNumSamples();
        const auto numChannels = (int) block.getNumChannels();
        const auto numBands = layout.numBands;

        const float crossoverFreqs[] { crossoverLow.getRaw (0), crossoverMid.getRaw (0), crossoverHigh.getRaw (0) };

        // the frequency of each split in the order they run, whichever crossovers the layout picks
        float cutoffs[maxSlots - 1] {};

        for (int i = 0; i < numBands - 1; ++i)
            cutoffs[i] = crossoverFreqs[layout.crossovers[(size_t) i]];

        if (linearCrossovers())
        {
            linearSplit.setCutoffs (cutoffs, numBands - 1);
            linearSplit.split (block, bandBuffers.data(), numBands - 1);
        }
        else
        {
            splitBands (block, numBands, cutoffs);
        }

        block.clear();

        for (int band = 0; band < numBands; ++band)
        {
            auto bandBlock = juce::dsp::AudioBlock<float> (bandBuffers[(size_t) band])
                                 .getSubBlock (0, (size_t) numSamples)
                                 .getSubsetChannelBlock (0, (size_t) numChannels);

            // everything past the split is the slot's: its distortion, its mute and solo, its scope tap
            const auto slot = layout.slots[(size_t) band];

            tapIn (slot, bandBlock);
            slots[(size_t) slot]->processBlock (bandBlock);
            tapOut (slot, bandBlock);

            if (slots[(size_t) slot]->isEnabled())
                pathBlockers[(size_t) slot].processBlock (bandBlock);

            // ramped, so a mute or solo fades rather than clicks
            bandLevels[(size_t) slot].setTargetValue (bandLevel (slot, layout));
            bandLevels[(size_t) slot].applyGain (bandBuffers[(size_t) band], numSamples);

            block.add (bandBlock);
        }
    }

    void splitBands (const juce::dsp::AudioBlock<float>& block, int numBands, const float* cutoffs)
    {
        const auto numSamples = (int) block.getNumSamples();
        const auto numChannels = (int) block.getNumChannels();

        for (int i = 0; i < numBands - 1; ++i)
        {
            splits[(size_t) i].setCutoffFrequency (cutoffs[i]);

            /*  A band that came off crossover i never passed through the crossovers after it, so
                it is missing their phase shift and won't sum flat with the bands that did. An
                allpass at each later crossover gives every band the same phase history. */
            for (int later = i + 1; later < numBands - 1; ++later)
            {
                allpasses[(size_t) i][(size_t) later].setCutoffFrequency (cutoffs[later]);
            }
        }

        std::array<std::array<float*, Crossover::maxChannels>, maxSlots> out {};

        for (int band = 0; band < numBands; ++band)
            for (int ch = 0; ch < numChannels; ++ch)
                out[(size_t) band][(size_t) ch] = bandBuffers[(size_t) band].getWritePointer (ch);

        // both channels inside the sample loop, see Crossover::processSample
        for (int n = 0; n < numSamples; ++n)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto remaining = block.getSample (ch, n);

                for (int band = 0; band < numBands - 1; ++band)
                {
                    float low = 0.0f, high = 0.0f;

                    splits[(size_t) band].processSample (ch, remaining, low, high);

                    for (int later = band + 1; later < numBands - 1; ++later)
                        low = allpasses[(size_t) band][(size_t) later].allpass (ch, low);

                    out[(size_t) band][(size_t) ch][n] = low;
                    remaining = high;
                }

                out[(size_t) (numBands - 1)][(size_t) ch][n] = remaining;
            }
        }
    }

    void processMidSide (juce::dsp::AudioBlock<float>& block)
    {
        if (block.getNumChannels() < 2)
            return processFirstSlot (block);

        const auto numSamples = (int) block.getNumSamples();

        auto* left = block.getChannelPointer (0);
        auto* right = block.getChannelPointer (1);

        for (int n = 0; n < numSamples; ++n)
        {
            const auto midGain = juce::Decibels::decibelsToGain (msBalance.getNextValue() * -0.5f);

            bandBuffers[0].setSample (0, n, (left[n] + right[n]) * 0.5f * midGain);
            bandBuffers[1].setSample (0, n, (left[n] - right[n]) * 0.5f / midGain);
        }

        for (int i = 0; i < 2; ++i)
        {
            auto mono = juce::dsp::AudioBlock<float> (bandBuffers[(size_t) i])
                            .getSubBlock (0, (size_t) numSamples)
                            .getSubsetChannelBlock (0, 1);

            tapIn (i, mono);
            slots[(size_t) i]->processBlock (mono);
            tapOut (i, mono);
            pathBlockers[(size_t) i].processBlock (mono);
        }

        for (int n = 0; n < numSamples; ++n)
        {
            const auto a = bandBuffers[0].getSample (0, n);
            const auto b = bandBuffers[1].getSample (0, n);

            left[n] = a + b;
            right[n] = a - b;
        }
    }

    void processExciter (juce::dsp::AudioBlock<float>& block)
    {
        const auto numSamples = (int) block.getNumSamples();
        const auto numChannels = (int) block.getNumChannels();
        const auto cutoff = crossoverLow.getRaw (0);

        auto dryBlock = juce::dsp::AudioBlock<float> (bandBuffers[0])
                            .getSubBlock (0, (size_t) numSamples)
                            .getSubsetChannelBlock (0, (size_t) numChannels);

        auto highBlock = juce::dsp::AudioBlock<float> (bandBuffers[1])
                             .getSubBlock (0, (size_t) numSamples)
                             .getSubsetChannelBlock (0, (size_t) numChannels);

        if (linearCrossovers())
        {
            linearSplit.setCutoffs (&cutoff, 1);
            linearSplit.split (block, bandBuffers.data(), 1);
        }
        else
        {
            splits[0].setCutoffFrequency (cutoff);

            for (int n = 0; n < numSamples; ++n)
            {
                for (int ch = 0; ch < numChannels; ++ch)
                {
                    float low = 0.0f, high = 0.0f;
                    splits[0].processSample (ch, block.getSample (ch, n), low, high);
                    dryBlock.setSample (ch, n, low);
                    highBlock.setSample (ch, n, high);
                }
            }
        }

        // the full range path is the split summed back
        dryBlock.add (highBlock);

        // the buffers are the paths, full range then high; everything run on them belongs to the path's slot
        const auto layout = bandLayout (exciter, 2);

        for (int path = 0; path < 2; ++path)
        {
            const auto slot = layout.slots[(size_t) path];
            auto& pathBlock = path == 0 ? dryBlock : highBlock;

            tapIn (slot, pathBlock);
            slots[(size_t) slot]->processBlock (pathBlock);
            tapOut (slot, pathBlock);

            if (slots[(size_t) slot]->isEnabled())
                pathBlockers[(size_t) slot].processBlock (pathBlock);

            bandLevels[(size_t) slot].setTargetValue (bandLevel (slot, layout));
            bandLevels[(size_t) slot].applyGain (bandBuffers[(size_t) path], numSamples);
        }

        block.copyFrom (dryBlock);
        block.add (highBlock);
    }

    // the scope's in/out trace, either side of the one slot being looked at. taken before the path blockers, so it's the slot alone
    void tapIn (int slot, juce::dsp::AudioBlock<float>& block)
    {
        if (slot == tapSlot)
            scope.capturePreDistortion (block.getChannelPointer (0), block.getNumSamples(), tapOversampling);
    }

    void tapOut (int slot, juce::dsp::AudioBlock<float>& block)
    {
        if (slot == tapSlot)
            scope.capturePostDistortion (block.getChannelPointer (0), block.getNumSamples(), tapOversampling);
    }
    
    float bandLevel (int slot, const BandLayout& layout) const
    {
        auto anySolo = false;

        for (int band = 0; band < layout.numBands; ++band)
        {
            const auto* solo = bandSolos[(size_t) layout.slots[(size_t) band]];
            anySolo = anySolo || (solo != nullptr && solo->get());
        }

        const auto muted = bandMutes[(size_t) slot] != nullptr && bandMutes[(size_t) slot]->get();
        const auto soloed = bandSolos[(size_t) slot] != nullptr && bandSolos[(size_t) slot]->get();

        return muted || (anySolo && !soloed) ? 0.0f : 1.0f;
    }

    PrimaryDistortion& stageFor (int stage)
    {
        return stage == 0 ? *slots[0] : *extraStackStages[(size_t) juce::jmin (stage - 1, maxSlots - 2)];
    }

    std::array<std::unique_ptr<PrimaryDistortion>, maxSlots> slots;
    std::array<std::unique_ptr<PrimaryDistortion>, maxSlots - 1> extraStackStages;

    juce::AudioParameterChoice* routing = nullptr;
    juce::AudioParameterInt* stackCount = nullptr;
    juce::AudioParameterInt* bandCount = nullptr;
    juce::AudioParameterBool* stackFlip = nullptr;
    juce::AudioParameterChoice* stackFilter = nullptr;

    std::array<juce::AudioParameterBool*, maxSlots> bandMutes {}, bandSolos {};
    std::array<juce::SmoothedValue<float>, maxSlots> bandLevels;

    SmoothParam crossoverLow, crossoverMid, crossoverHigh, stackGain, stackMix, stackFilterFreq, stackFilterQ, stackRotation, msBalance;

    juce::dsp::DryWetMixer<float> stackDryWet;
    double sampleRate = 44100.0;

    // the dry's rotations stay at 0
    StackPath wetPath, dryPath;

    /*  One per parallel path, and per stack stage. Any offset a slot introduces would otherwise
        be summed into the output alongside the other paths, or -- in a stack -- fed into the next
        stage's shaper, where it compounds pass after pass. */
    std::array<DCBlockingHighPass, maxSlots> pathBlockers;

    // the stack's per stage blockers again, on the stack mix's dry
    std::array<DCBlockingHighPass, maxSlots> dryBlockers;

    std::array<Crossover, maxSlots - 1> splits;
    LinearPhaseSplit linearSplit;

    // [band][crossover]: only entries where crossover > band are ever used
    std::array<std::array<Crossover, maxSlots - 1>, maxSlots - 1> allpasses;
    std::array<juce::AudioBuffer<float>, maxSlots> bandBuffers;

    // may come from whichever thread loads the state, hence the atomics the audio thread reads
    void syncSettings()
    {
        crossoverSlope.store (crossoverSlopeFrom (apvts.state.getProperty (crossoverSlopeProperty)), std::memory_order_relaxed);
        linearHighpass.store ((bool) apvts.state.getProperty (linearHighpassProperty), std::memory_order_relaxed);
        dryPhase.store ((bool) apvts.state.getProperty (dryPhaseProperty, true), std::memory_order_relaxed);
    }

    void valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property) override
    {
        if (tree == apvts.state && (property == crossoverSlopeProperty || property == linearHighpassProperty || property == dryPhaseProperty))
            syncSettings();
    }

    void valueTreeRedirected (juce::ValueTree&) override { syncSettings(); }

    juce::AudioProcessorValueTreeState& apvts;
    std::atomic<int> crossoverSlope { 24 };
    std::atomic<bool> linearHighpass { false };
    std::atomic<bool> dryPhase { true };

    ScopeDataCollector<float>& scope;
    int tapSlot = -1, tapOversampling = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainRouting)
};
