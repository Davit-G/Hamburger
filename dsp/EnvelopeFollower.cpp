
#include "EnvelopeFollower.h"
#include <cmath> // std::abs
#include <JuceHeader.h>

float EnvelopeFollower::processSample(float xn)
{
    // full wave rectify signal
    float input = std::abs(xn);

    // square signal for RMS
    if (useRMS)
    {
        input *= input;
    }

    float currEnvelope = 0.0;

    // compression section
    if (input > lastEnvelope)
    {
        currEnvelope = attackTime * (lastEnvelope - input) + input;
    }
    else
    {
        currEnvelope = releaseTime * (lastEnvelope - input) + input;
    }

    // do we need to check underflow?
    // checkFloatUnderflow(currEnvelope)

    // if we want, clip at 0DB
    if (false)
    {
        currEnvelope = fmin(currEnvelope, 1.0);
    }

    // why would an envelope be below 0. It won't. so clip it.
    currEnvelope = fmax(currEnvelope, 0.0);

    // store envelope for future
    lastEnvelope = currEnvelope;

    // then some random RMS bullshittery
    if (useRMS)
    {
        currEnvelope = pow(currEnvelope, 0.5);
    }

    if (!useLog) // just skip the log stuff for now, make it into var later
    {
        return currEnvelope;
    }

    // --- setup for log( )
    if (currEnvelope <= 0)
        return -96.0;
    // --- true log output in dB, can go above 0dBFS!
    return 20.0 * log10(currEnvelope);
}

float EnvelopeFollower::processSampleStereo(float xL, float xR)
{
    // full wave rectify stereo signal and sum
    float input = (std::abs(xL) + std::abs(xR)) * 0.5;

    // square signal for RMS
    if (useRMS)
    {
        input *= input;
    }

    float currEnvelope = 0.0;

    // compression section
    if (input > lastEnvelope)
    {
        currEnvelope = attackTime * (lastEnvelope - input) + input;
    }
    else
    {
        currEnvelope = releaseTime * (lastEnvelope - input) + input;
    }

    

    // do we need to check underflow?
    // checkFloatUnderflow(currEnvelope)

    // if we want, clip at 0DB
    if (false)
    {
        currEnvelope = fmin(currEnvelope, 1.0);
    }

    // why would an envelope be below 0. It won't. so clip it.
    currEnvelope = fmax(currEnvelope, 0.0);

    // store envelope for future
    lastEnvelope = currEnvelope;

    // then some random RMS bullshittery

    if (useRMS)
    {
        currEnvelope = pow(currEnvelope, 0.5);
    }

    if (!useLog) // just skip the log stuff for now, make it into var later
    {
        return currEnvelope;
    }

    // --- setup for log( )
    if (currEnvelope <= 0)
        return -96.0;
    // --- true log output in dB, can go above 0dBFS!
    return 20.0 * log10(currEnvelope);
}

void EnvelopeFollower::prepareToPlay(double sr, int samplesPerBlock)
{
    // set correct sample rate information
    setSampleRate(sr);
    lastEnvelope = 0.0;
};