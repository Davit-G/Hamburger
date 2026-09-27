#pragma once
#include "../SmoothParam.h"

#if PERFETTO
#include <melatonin_perfetto/melatonin_perfetto.h>
#endif // PERFETTO
#include "../../utils/Params.h"

#include "../../gui/Modules/ScopeDataCollector.h"

#include "../EffectBase.h"

class PostClip : public MacroEffect
{
public:
    PostClip(juce::AudioProcessorValueTreeState& treeState, ScopeDataCollector<float>& scopeDataCollector);

    ~PostClip();

    void processBlock(juce::dsp::AudioBlock<float>& block) override;
    void prepare(juce::dsp::ProcessSpec& spec) override;

    // the limiter's lookahead, while it's the one running
    int getLatencySamples() const override;

private:
    enum Type { soft, limiter };

    /*  A brickwall limiter with a little lookahead. The gain each sample needs to stay under 0db is held at its lowest across
        the lookahead and a while after, let go in dB at the release time, and averaged across the lookahead again, while the
        audio waits the length of it. An average of held lows is already low enough when the peak comes out, so nothing gets
        past 0db, and every change being spread across the lookahead keeps it from adding much in the way of harmonics.

        The hold is longer than half a cycle of the lowest bass, so the gain doesn't start back up between one half cycle's
        peak and the next, which would wobble it at twice the bass and grind everything else riding on it. */
    struct Limiter
    {
        static constexpr double lookaheadSeconds = 0.0015, holdSeconds = 0.02;

        void prepare (double newSampleRate)
        {
            sampleRate = newSampleRate;
            length = juce::jmax (1, juce::roundToInt (sampleRate * lookaheadSeconds));
            held = length + juce::roundToInt (sampleRate * holdSeconds);
            waiting.assign ((size_t) (length * 2), 0.0f);
            recent.assign ((size_t) length, 1.0f);
            queue.assign ((size_t) held + 2, {});
            reset();
        }

        // silence waiting and nothing held, for when it starts limiting again after being something else
        void reset()
        {
            std::fill (waiting.begin(), waiting.end(), 0.0f);
            std::fill (recent.begin(), recent.end(), 1.0f);
            sum = length;
            released = 1.0f;
            position = front = count = 0;
            time = 0;
        }

        void setRelease (float ms) { release = 1.0f - std::exp (-1000.0f / (juce::jmax (0.1f, ms) * (float) sampleRate)); }

        // both channels together, so the image doesn't move
        void process (float& left, float& right)
        {
            const auto peak = juce::jmax (std::abs (left), std::abs (right));
            const auto needed = peak > 1.0f ? 1.0f / peak : 1.0f;

            // back toward nothing by the same share of the dB left each sample, however deep it went
            released = juce::jmin (lowest (needed), std::pow (released, 1.0f - release));

            const auto slot = (size_t) position;
            sum += released - recent[slot];
            recent[slot] = released;
            const auto gain = (float) (sum / length);

            // the audio waits the lookahead, each channel's in its own half
            for (auto [channel, x] : { std::pair<size_t, float*> { 0, &left }, std::pair<size_t, float*> { 1, &right } })
            {
                auto& waited = waiting[channel * (size_t) length + slot];
                const auto late = waited;
                waited = *x;
                *x = late * gain;
            }

            position = (position + 1) % length;
        }

        int length = 1;

    private:
        struct Held { juce::int64 time; float gain; };

        /*  The lowest needed gain of the last lookahead and hold, and one, which is the one sample more the average reaches back
            past a peak's own. Kept as a queue of those that could still be the lowest, each lower than all before it. */
        float lowest (float needed)
        {
            const auto capacity = (int) queue.size();
            auto at = [&] (int i) -> Held& { return queue[(size_t) ((front + i) % capacity)]; };

            while (count > 0 && at (count - 1).gain >= needed)
                --count;

            at (count++) = { time, needed };

            while (at (0).time < time - held)
            {
                front = (front + 1) % capacity;
                --count;
            }

            ++time;
            return at (0).gain;
        }

        double sampleRate = 44100.0, sum = 1.0;
        float release = 0.0f, released = 1.0f;
        int held = 1, position = 0, front = 0, count = 0;
        juce::int64 time = 0;

        std::vector<float> waiting, recent;
        std::vector<Held> queue;
    };

    SmoothParam gainKnob;
    SmoothParam kneeKnob;
    SmoothParam timeKnob;

    Limiter lookahead;
    bool wasLimiting = false;

    juce::AudioParameterBool *clipEnabled;
    juce::AudioParameterChoice *type;

    ScopeDataCollector<float>& scopeData;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PostClip)
};