
#include "EnvelopeFollower.h"
#include <cmath> // std::abs

#include <JuceHeader.h>

float EnvelopeFollower::processSample(float xn)
{

    if (treeStateRef != nullptr) {
        // get knobs
        float attackT = knobValue->get();
        float releaseT = knobValue2->get();

        // set attack and release times
        setAttackTime(attackT);
        setReleaseTime(releaseT);
    }

    // full wave rectify signal
    float input = std::abs(xn);

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

void EnvelopeFollower::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // set correct sample rate information
    setSampleRate(sampleRate);
    lastEnvelope = 0.0;
    setAttackTime(10);
    setReleaseTime(190);

    if (treeStateRef == nullptr)
        return;
    
    knobValue = dynamic_cast<juce::AudioParameterFloat *>(treeStateRef->getParameter("compAttack"));
    jassert(knobValue); // makes sure it exists

    knobValue2 = dynamic_cast<juce::AudioParameterFloat *>(treeStateRef->getParameter("compRelease"));
    jassert(knobValue2); // makes sure it exists
};