
#include "EnvelopeFollower.h"
#include <cmath> // std::abs
#include "juce_core/juce_core.h"

float EnvelopeFollower::processSample(float xn)
{
    // full wave rectify signal
    float input = abs(xn);

    // square signal for RMS
    input *= input;

    // pick between rising or falling value
    float currEnvelope = 0.0f;
    float diff = lastEnvelope - input;
    
    if (input > lastEnvelope) {
        currEnvelope = attackTime * diff + input;
    } else {
        currEnvelope = releaseTime * diff + input;
    }

    currEnvelope = fmax(currEnvelope, 0.0f);
    lastEnvelope = currEnvelope;
    currEnvelope = pow(currEnvelope, 0.5f);

    if (!useLog) // just skip the log stuff for now, make it into var later
    {
        return currEnvelope;
    }

    // --- setup for log( )
    if (currEnvelope <= 0.0f)
        return -96.0f;
    // --- true log output in dB, can go above 0dBFS!
    return 20.0f * log10(currEnvelope);
}

float EnvelopeFollower::processSampleStereo(float xL, float xR)
{
    // full wave rectify stereo signal and sum
    float input = (abs(xL) + abs(xR)) * 0.5f;

    input *= input; // square signal for RMS
    
    // pick between rising or falling value
    float currEnvelope = 0.0f;
    float diff = lastEnvelope - input;
    if (input > lastEnvelope) {
        currEnvelope = attackTime * diff + input;
    } else {
        currEnvelope = releaseTime * diff + input;
    }

    // why would an envelope be below 0. It won't. so clip it.
    currEnvelope = fmax(currEnvelope, 0.0f);
    lastEnvelope = currEnvelope; // store envelope for future
    currEnvelope = pow(currEnvelope, 0.5f); // then some random RMS bullshittery

    if (!useLog) // just skip the log stuff for now, make it into var later
    {
        return currEnvelope;
    }

    // --- setup for log( )
    if (currEnvelope <= 0.0f)
        return -96.0f;
    
    // --- true log output in dB, can go above 0dBFS!
    return 20.0f * log10(currEnvelope);
}

void EnvelopeFollower::prepare(juce::dsp::ProcessSpec& spec)
{
    // set correct sample rate information
    setSampleRate(spec.sampleRate);
    lastEnvelope = 0.0f;
}