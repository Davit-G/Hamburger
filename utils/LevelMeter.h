#pragma once

#include <atomic>
#include <cmath>

#include <juce_core/juce_core.h>

// used between audio and gui thread
// uses decay to reliably have a changing value at 60hz while also keeping up with the exact audio signal
// so that the drawing isnt slower than block size updates
class LevelMeter {
public:
    LevelMeter(float decay = 0.07f) {
        decayRate = decay;
    }

    ~LevelMeter() {}

    // to be used on gui thread at high refresh rate
    // only call this once in the same frame
    float getNext() {
        const auto now = juce::Time::getMillisecondCounterHiRes();
        const auto elapsed = lastDecayMs > 0.0 ? juce::jlimit(0.0, 1.0, (now - lastDecayMs) * 0.001) : 0.0;

        lastDecayMs = now;

        const auto decayed = sanitised(value.load(std::memory_order_relaxed))
                                 * std::pow(1.0f - decayRate, (float) elapsed * 60.0f);

        value.store(decayed, std::memory_order_relaxed);
        return decayed;
    }

    // can be used anywhere
    float getCurrent() {
        return sanitised(value.load(std::memory_order_relaxed));
    }

    // similar to below but uses db
    void accumulateDb(float db) {
        accumulate(std::pow(10.0f, db * 0.05f));
    }

    // designed to be used in audio hot loop and given a positive abs value
    void accumulate(float smp) {
        float sample = std::abs(smp);

        if (!std::isfinite(sample))
            return;

        if (sample > sanitised(value.load(std::memory_order_relaxed))) {
            value.store(std::min(sample, maxLevel), std::memory_order_relaxed);
        }
    }

    void reset() {
        value.store(0, std::memory_order_relaxed);
    }

private:
    static float sanitised(float level) {
        return std::isfinite(level) && level > 0.0f ? std::min(level, maxLevel) : 0.0f;
    }
    
    std::atomic<float> value { 0.0f };
    double lastDecayMs = 0.0;
    float decayRate = 0.001f;

    static constexpr float maxLevel = 10.0f;
};