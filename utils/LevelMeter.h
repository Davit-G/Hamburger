#pragma once

#include <atomic>
#include <cmath>

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
    float getNext(float frameRate) {
        float current = value.load(std::memory_order_relaxed);
        value.store(current * (1.0f - (decayRate * 60.0f / frameRate)), std::memory_order_relaxed);
        return value.load(std::memory_order_relaxed);
    }

    // can be used anywhere
    float getCurrent() {
        return value.load(std::memory_order_relaxed);
    }

    // similar to below but uses db
    void accumulateDb(float db) {
        accumulate(std::pow(10.0f, db * 0.05f));
    }

    // designed to be used in audio hot loop and given a positive abs value
    void accumulate(float smp) {
        float sample = abs(smp);
        float current = value.load(std::memory_order_relaxed);

        if (!std::isfinite(current) && current > maxLevel)
            current = 0.0f;

        if (std::isfinite(sample) && sample > current) {
            // jump
            value.store(sample, std::memory_order_relaxed);
        }
    }

private:
    std::atomic<float> value;
    float decayRate = 0.001f;
    float maxLevel = 10.0f;
};