#pragma once

#include <cmath>
#include "juce_core/juce_core.h"

/*  A follower worked as a second order system, critically damped unless it's asked otherwise: what it follows has a speed of
    its own that eases into every change, so its slope never jumps, even where it switches from rising to falling. Below a
    damping of 1 it overshoots and rings back, above it creeps in. Its speed is in level per sample. */
class DampedFollower
{
public:
    // a critically damped follower gets 63% of the way in 2.146 of its time constants, so it runs that much quicker to
    // arrive when a one pole of the same time would
    static constexpr float sameArrival = 2.146f;

    void prepare (double newSampleRate)
    {
        sampleRate = (float) newSampleRate;
        level = speed = 0.0f;
    }

    void setTimes (float attackMs, float releaseMs, float damping = 1.0f)
    {
        attack = ratesFor (attackMs, damping);
        release = ratesFor (releaseMs, damping);
    }

    /*  Pulled toward the input and held back by its own speed. The holding back is worked from the new speed rather than the
        old, which keeps it steady however heavy the damping or short the time, where working it from the old one blows up
        once damping outweighs the step. Stopped dead at nothing, so no speed is left over to work off before it can rise. */
    float process (float input)
    {
        const auto& rates = input > level ? attack : release;
        speed = (speed + rates.pull * (input - level)) / (1.0f + rates.damping);
        level += speed;

        if (level < 0.0f)
            level = speed = 0.0f;

        return level;
    }

private:
    struct Rates { float pull = 1.0f, damping = 2.0f; };

    Rates ratesFor (float ms, float damping) const
    {
        const auto perSample = sameArrival * 1000.0f / (juce::jmax (0.1f, ms) * sampleRate);
        return { perSample * perSample, 2.0f * damping * perSample };
    }

    float sampleRate = 44100.0f;
    Rates attack, release;
    float level = 0.0f, speed = 0.0f;
};

// a light dependent resistor with the timing of an LA-2A's: fitted to recordings of one, it lets go about ten and a half
// times slower than it lights up
class OptoCell : public DampedFollower
{
public:
    static constexpr float releasePerAttack = 10.5f;

    // always a little behind, so at 0ms there's still something for the level to get ahead of
    void setSpeed (float ms, float damping = 1.0f) { setTimes (ms + 1.0f, ms * releasePerAttack + 1.0f, damping); }
};
