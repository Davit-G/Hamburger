#pragma once

#include "../../gui/Modules/ScopeDataCollector.h"
#include "../EffectBase.h"
#include "../EffectInfos.h"
#include "ThreeBands.h"
#include "../OptoCell.h"

/*  The level held at its peaks against an opto cell of it. Ahead of the cell is the start of a hit, and behind it is what
    rings on as the cell lets go. Either is turned into a gain in dB, in full once it's this far off. */
class TransientDetector
{
public:
    static constexpr float fullDb = 12.0f;

    void prepare (juce::dsp::ProcessSpec& spec)
    {
        sampleRate = (float) spec.sampleRate;
        quick = 0.0f;
        opto.prepare (spec.sampleRate);
    }

    // at 0ms the peaks aren't held at all, so it shapes each cycle of the waveform
    void setSpeed (float ms)
    {
        quickRelease = ms > 0.0f ? std::exp (-1000.0f / (ms * sampleRate)) : 0.0f;
        opto.setSpeed (ms);
    }

    // also keeps whichever gain this block has been furthest from nothing, for the scope
    float gainDb (float level, float attackDb, float sustainDb)
    {
        quick = juce::jmax (level, quick * quickRelease);

        const auto ahead = (juce::Decibels::gainToDecibels (quick) - juce::Decibels::gainToDecibels (opto.process (level))) / fullDb;
        const auto db = attackDb * juce::jlimit (0.0f, 1.0f, ahead) + sustainDb * juce::jlimit (0.0f, 1.0f, -ahead);

        if (std::abs (db) > std::abs (furthestDb))
            furthestDb = db;

        return db;
    }

    // hands the scope the furthest gain unless it's still to read a further one
    void showOn (std::atomic<float>& shown)
    {
        if (std::abs (furthestDb) > std::abs (shown.load (std::memory_order_relaxed)))
            shown.store (furthestDb, std::memory_order_relaxed);

        furthestDb = 0.0f;
    }

private:
    OptoCell opto;
    float sampleRate = 44100.0f, quick = 0.0f, quickRelease = 0.0f, furthestDb = 0.0f;
};

class TransientShaper : public MacroEffect
{
public:
    TransientShaper (juce::AudioProcessorValueTreeState& state, ScopeDataCollector<float>& dataCollector)
        : MacroEffect (state, SlotId { ModuleId::dynamics, 0 }),
          attack (getParam (ParamIDs::transientAttack)),
          sustain (getParam (ParamIDs::transientSustain)),
          speed (getParam (ParamIDs::transientSpeed)),
          link (getParam (ParamIDs::transientLink)),
          scopeDataCollector (dataCollector) {}

    void processBlock (juce::dsp::AudioBlock<float>& block) override
    {
        detectorL.setSpeed (speed.getRaw (0));
        detectorR.setSpeed (speed.getRaw (0));

        const auto attackDb = attack.getRaw (0);
        const auto sustainDb = sustain.getRaw (0);
        const auto linked = link.getRaw (0) * 0.01f;

        for (size_t sample = 0; sample < block.getNumSamples(); ++sample)
        {
            const auto left = block.getSample (0, (int) sample);
            const auto right = block.getSample (1, (int) sample);

            // each side hears itself, or both together at full link, so at full link they get the same gain
            const auto both = (std::abs (left) + std::abs (right)) * 0.5f;
            const auto heardL = std::abs (left) + (both - std::abs (left)) * linked;
            const auto heardR = std::abs (right) + (both - std::abs (right)) * linked;

            block.setSample (0, (int) sample, left * juce::Decibels::decibelsToGain (detectorL.gainDb (heardL, attackDb, sustainDb)));
            block.setSample (1, (int) sample, right * juce::Decibels::decibelsToGain (detectorR.gainDb (heardR, attackDb, sustainDb)));
        }

        detectorL.showOn (scopeDataCollector.transientGainDb[0]);
        detectorR.showOn (scopeDataCollector.transientGainDb[1]);
    }

    void prepare (juce::dsp::ProcessSpec& spec) override
    {
        detectorL.prepare (spec);
        detectorR.prepare (spec);
    }

private:
    SmoothParam attack, sustain, speed, link;
    TransientDetector detectorL, detectorR;

    ScopeDataCollector<float>& scopeDataCollector;
};

// split into lows, mids and highs, with the tilt taking the attack and sustain away from one end and adding it to the other
class MBTransientShaper : public MacroEffect
{
public:
    MBTransientShaper (juce::AudioProcessorValueTreeState& state, ScopeDataCollector<float>& dataCollector, ThreeBands& threeBands)
        : MacroEffect (state, SlotId { ModuleId::dynamics, 0 }),
          attack (getParam (ParamIDs::transientAttack)),
          sustain (getParam (ParamIDs::transientSustain)),
          tilt (getParam (ParamIDs::transientTilt)),
          speed (getParam (ParamIDs::transientSpeed)),
          scopeDataCollector (dataCollector),
          bands (threeBands) {}

    void processBlock (juce::dsp::AudioBlock<float>& block) override
    {
        for (auto& detector : detectors)
            detector.setSpeed (speed.getRaw (0));

        const auto tilted = tilt.getRaw (0) * 0.01f;
        const std::array<float, 3> bandAmounts { 1.0f - tilted, 1.0f, 1.0f + tilted };

        const auto attackDb = attack.getRaw (0);
        const auto sustainDb = sustain.getRaw (0);

        bands.split (block);

        for (int sample = 0; sample < (int) block.getNumSamples(); ++sample)
        {
            auto outL = 0.0f, outR = 0.0f;

            for (size_t i = 0; i < detectors.size(); ++i)
            {
                const auto left = bands.bands[i].getSample (0, sample);
                const auto right = bands.bands[i].getSample (1, sample);
                const auto level = (std::abs (left) + std::abs (right)) * 0.5f;
                const auto gain = juce::Decibels::decibelsToGain (detectors[i].gainDb (level, attackDb * bandAmounts[i], sustainDb * bandAmounts[i]));

                outL += left * gain;
                outR += right * gain;
            }

            block.setSample (0, sample, outL);
            block.setSample (1, sample, outR);
        }

        for (size_t i = 0; i < detectors.size(); ++i)
            detectors[i].showOn (scopeDataCollector.transientGainDb[i]);
    }

    void prepare (juce::dsp::ProcessSpec& spec) override
    {
        for (auto& detector : detectors)
            detector.prepare (spec);
    }

private:
    SmoothParam attack, sustain, tilt, speed;

    std::array<TransientDetector, 3> detectors;

    ScopeDataCollector<float>& scopeDataCollector;
    ThreeBands& bands;
};
