#pragma once

#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_dsp/juce_dsp.h"

#include "../../DCBlockingHighPass.h"
#include "../../EffectBase.h"
#include "../../SmoothParam.h"
#include "WaveshapeMap.h"
#include "WaveshapeOrder.h"

namespace waveshapes
{
    // this is the filter group used to select between waveshape types. there are 4 groups.
    inline constexpr unsigned allGroups = (1u << GroupCount) - 1u;

    inline juce::Identifier groupFilterProperty (SlotId slot) { return slot.prefix() + "_waveshapeGroups"; }

    inline unsigned groupFilterFrom (const juce::var& stored)
    {
        const auto groups = (stored.isVoid() ? allGroups : (unsigned) (int) stored) & allGroups;
        return groups != 0 ? groups : allGroups;
    }

    struct FilteredMap
    {
        std::array<MapPoint, std::size (mapPoints)> points {};
        int count = 0;
        unsigned groups = 0; // none yet, so the first update always builds

        // true when it changed
        bool update (unsigned newGroups)
        {
            if (newGroups == groups)
                return false;

            groups = newGroups;
            count = 0;

            for (const auto& point : mapPoints)
                if ((groups & (1u << shapes[(size_t) point.index].group)) != 0)
                    points[(size_t) count++] = point;

            return true;
        }

        Mix mixAt (double x, double y) const { return waveshapes::mixAt (points.data(), count, x, y); }
    };
}

class Waveshape : public MacroEffect,
                  private juce::ValueTree::Listener
{
public:
    struct Settings
    {
        double x = -1.0, y = -1.0, width = 0.0; // x -1 is off the pad, so the first block always builds

        bool operator== (const Settings& other) const
        {
            return x == other.x && y == other.y && width == other.width;
        }
    };

    /*  The curve a waveshape plays, built once and read by every instance running the same slot's settings. The
        stack's stages are all slot 1, so one build serves them all rather than one each, which matters most for
        smooth: a smoothed table is four box filter passes over 8192 cells.

        Double buffered: a change builds into whichever table the last curve isn't in, so for the block it happens
        in, every instance has both the old curve and the new one to fade between. generation counts the builds, so
        an instance that sat out a change can tell whether its old table is still intact. Audio thread only, one
        instance after another within a block, so it needs no locking. */
    struct SharedCurve
    {
        Settings settings;
        waveshapes::FilteredMap filtered;
        waveshapes::Mix mix;
        std::array<waveshapes::CurveTable, 2> tables;
        int current = -1;          // which table holds the current curve, none before the first build
        juce::uint64 generation = 0;

        void update (const Settings& wanted, unsigned groups)
        {
            const auto filterChanged = filtered.update (groups);

            if (current >= 0 && ! filterChanged && wanted == settings)
                return;

            settings = wanted;
            mix = filtered.mixAt (wanted.x, wanted.y);
            current = current == 0 ? 1 : 0;
            tables[(size_t) current].build (mix, wanted.width);
            ++generation;
        }
    };

    // stages sharing a slot share one curve
    Waveshape (juce::AudioProcessorValueTreeState& state, SlotId slot, std::shared_ptr<SharedCurve> sharedCurve = nullptr)
        : MacroEffect (state, slot),
          shared (sharedCurve != nullptr ? std::move (sharedCurve) : std::make_shared<SharedCurve>()),
          apvts (state),
          filterProperty (waveshapes::groupFilterProperty (slot)),
          drive (getParam (ParamIDs::waveshapeDrive)),
          padX (getParam (ParamIDs::waveshapeX)),
          padY (getParam (ParamIDs::waveshapeY)),
          smooth (getParam (ParamIDs::waveshapeSmooth)),
          bias (getParam (ParamIDs::waveshapeBias)),
          asym (getParam (ParamIDs::waveshapeAsym))
    {
        jassert ((double) ParamIDs::waveshapeDrive.range.end <= waveshapes::maxDriveDb);

       #if JUCE_DEBUG
        // tables edited without regenerating the headers would quietly swap curves under every preset
        jassert (std::size (waveshapes::loudnessGains) == waveshapes::shapes.size());
        jassert (std::size (waveshapes::mapPoints) == waveshapes::shapes.size());

        for (const auto& point : waveshapes::mapPoints)
            jassert (point.index < (int) waveshapes::shapes.size()
                     && std::strcmp (waveshapes::shapes[(size_t) point.index].name, point.name) == 0);
       #endif

        // the state tree gets swapped whole on a preset or project load, which is valueTreeRedirected
        apvts.state.addListener (this);
        syncGroupFilter();
    }

    ~Waveshape() override { apvts.state.removeListener (this); }

    void prepare (juce::dsp::ProcessSpec& spec) override
    {
        drive.prepare (spec);
        bias.prepare (spec);
        asym.prepare (spec);
        highpass.prepare (spec);

        // the next block starts the curve afresh rather than fading in from a stale one
        now = {};
        before = {};
        rests.assign (spec.numChannels, {});
    }

    void processBlock (juce::dsp::AudioBlock<float>& block) override
    {
        drive.update();
        bias.update();
        asym.update();

        const Settings wanted { (double) padX.getRaw(),
                                (double) padY.getRaw(),
                                waveshapes::smoothToWidth (smooth.getRaw() * 0.01) };

        // whichever instance gets here first on a change builds it, the rest find it built. a new filter changes the
        // blend at the same spot, so it crossfades in the same as a move would
        shared->update (wanted, groupFilter.load (std::memory_order_relaxed));

        const bool changed = now.generation != shared->generation;

        // the outgoing table is only still intact if this instance was playing the curve right before this one
        const bool fade = changed && now.table != nullptr && now.generation + 1 == shared->generation;

        // re-levelling depends on the drive too, but gently, so it's redone per block only once drive has moved relevelStepDb
        const auto blockDriveDb = (double) drive.getRaw();

        if (changed)
        {
            before = now;
            now.mix = shared->mix;
            now.relevel = waveshapes::mixRelevel (now.mix, blockDriveDb, waveshapes::loudnessGains);
            now.relevelDriveDb = blockDriveDb;
            now.table = &shared->tables[(size_t) shared->current];
            now.generation = shared->generation;

            for (auto& rest : rests)
                rest.valid = false;
        }
        else if (std::abs (blockDriveDb - now.relevelDriveDb) >= relevelStepDb)
        {
            now.relevel = waveshapes::mixRelevel (now.mix, blockDriveDb, waveshapes::loudnessGains);
            now.relevelDriveDb = blockDriveDb;
        }

        const auto numSamples = block.getNumSamples();
        const auto numChannels = juce::jmin (block.getNumChannels(), rests.size());

        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            auto* samples = block.getChannelPointer (ch);
            auto& rest = rests[ch];
            const auto c = (int) ch;

            for (size_t n = 0; n < numSamples; ++n)
            {
                const auto driveDb = (double) drive.getNextValue (c);
                const auto gain = (double) juce::Decibels::decibelsToGain (driveDb);
                const auto offset = waveshapes::biasCurve (bias.getNextValue (c));
                const auto amount = asym.getNextValue (c) * 0.01;

                const auto in = std::tanh (waveshapes::skew (samples[n] * gain + offset, amount));

                // where the bias alone lands only moves while bias, asym or the curve does
                if (! rest.valid || offset != rest.offset || amount != rest.amount)
                    rest = { true, offset, amount, now.at (std::tanh (waveshapes::skew (offset, amount))) };

                auto shaped = (now.at (in) - rest.output) * now.loudness (driveDb);

                if (fade)
                {
                    const auto old = (before.at (in) - before.at (std::tanh (waveshapes::skew (offset, amount))))
                                     * before.loudness (driveDb);
                    shaped = old + (shaped - old) * (double) (n + 1) / (double) numSamples;
                }

                samples[n] = (float) shaped;
            }
        }

        highpass.processBlock (block);
    }

private:
    static constexpr double relevelStepDb = 0.5;

    // may come from whichever thread loads the state, hence the atomic the audio thread reads
    void syncGroupFilter() { groupFilter.store (waveshapes::groupFilterFrom (apvts.state.getProperty (filterProperty)), std::memory_order_relaxed); }

    void valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property) override
    {
        if (tree == apvts.state && property == filterProperty)
            syncGroupFilter();
    }

    void valueTreeRedirected (juce::ValueTree&) override { syncGroupFilter(); }

    std::shared_ptr<SharedCurve> shared;

    juce::AudioProcessorValueTreeState& apvts;
    const juce::Identifier filterProperty;
    std::atomic<unsigned> groupFilter { waveshapes::allGroups };

    SmoothParam drive, padX, padY, smooth, bias, asym;

    struct Curve
    {
        juce::uint64 generation = 0; // used to check changes
        waveshapes::Mix mix;
        double relevel = 1.0, relevelDriveDb = 0.0;
        const waveshapes::CurveTable* table = nullptr;

        double at (double x) const
        {
            return table != nullptr ? table->read (x) : waveshapes::mixCurve (mix, x);
        }

        double loudness (double driveDb) const
        {
            return waveshapes::mixLoudness (mix, driveDb, waveshapes::loudnessGains) * relevel;
        }
    };

    // the curve playing now, and the one it replaced, which a change's block fades out of. both tables are the shared curve's
    Curve now, before;

    struct Rest
    {
        bool valid = false;
        double offset = 0.0, amount = 0.0, output = 0.0;
    };

    std::vector<Rest> rests;

    DCBlockingHighPass highpass;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Waveshape)
};
