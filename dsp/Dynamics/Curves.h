#pragma once
 
#include <cmath>
#include <JuceHeader.h>

using namespace juce;


class Curves {
public:
    static float computeCompressorGain(float input_db, float threshold, float ratio, float kneeWidth);
    static float computeExpanderGain(float input_db, float ratio, float kneeWidth);
    static float computeUpwardsDownwardsGain(float x, float upperThres, float lowerThres, float upperR, float lowerR, float knee);
};

float Curves::computeCompressorGain(float input_db, float threshold, float ratio, float kneeWidth)
{
    // this is the soft-clip function from the compressor
    // https://www.desmos.com/calculator/f8zazgtwpe

    // input is supposed to be in DB
    // output is supposed to be in gain

    float output = -100.0f;

    if (2 * (input_db - threshold) < -kneeWidth)
    {
        output = input_db;
    }
    else if (2 * (input_db - threshold) > kneeWidth)
    {
        output = threshold + (input_db - threshold) / ratio;
    }
    else
    {
        output = input_db + ((1.0f / ratio - 1.0f) * pow((input_db - threshold + kneeWidth * 0.5f), 2.0f) / (2.0f * kneeWidth));
    }

    float gainReduction = 0.f;

    gainReduction = output - input_db;

    return juce::Decibels::decibelsToGain(gainReduction);
}

float Curves::computeExpanderGain(float input_db, float ratio, float kneeWidth)
{
    float output = -100.0f;
    float thres = 0.f;

    if (2 * (input_db - thres) > kneeWidth) {
        // below knee
        output = input_db;
    }
    else if (2 * (input_db - thres) <= -kneeWidth) {
        output = thres + (input_db - thres) * ratio;
    } else {
        output = input_db + ((1 / ratio) * pow((input_db - thres + kneeWidth * 0.5f), 2.0f) / (2.0f * kneeWidth));
    }

    float gainReduction = 0.f;
    gainReduction = output - input_db;
    return juce::Decibels::decibelsToGain(gainReduction);
}

float Curves::computeUpwardsDownwardsGain(float x, float upperThres, float lowerThres, float upperR, float lowerR, float knee) {
    float output = -100.0f;

    float a = upperThres - 2*knee;
    if (lowerThres >= a) {
        lowerThres = a;
    }

    // 2 * abs(x - lowerThres)
    float b = 2 * abs(x - lowerThres);

    if (2 * (x - upperThres) > knee) {
        // line above knee for top part
        output = upperThres + (x - upperThres) / upperR;
    } else if (2 * (abs(x - upperThres)) <= knee) {
        // top knee
        output = x + ((1 / upperR - 1) * pow(x - upperThres + (knee * 0.5f), 2.0f)) / (2.0f * knee);
    } else if (b <= knee) {
        // bottom knee
        output = x + ((1 - 1 / lowerR) * pow(x - lowerThres - (knee * 0.5f), 2.0f)) / (2.0f * knee);
    } else if (b < -knee) {
        // line on bottom knee
        output = lowerThres + (x - lowerThres) / lowerR;
    // } else if (knee / 2 + lowerThres < x && x < (-knee / 2) + upperThres) {
    } else {
        // in between the two thresholds
        output = x;
    } 

    float gainReduction = 0.f;
    gainReduction = output - x;
    return juce::Decibels::decibelsToGain(gainReduction);
}