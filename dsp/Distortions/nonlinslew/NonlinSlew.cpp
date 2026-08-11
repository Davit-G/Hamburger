#include "NonlinSlew.h"
#include "../../Waveshapers.h"

#include <algorithm>
#include <cmath>

namespace {

float safePow(float value, float exponent) {
    const float magnitude = std::max(std::fabs(value), 1.0e-6f);
    return std::pow(magnitude, exponent);
}

} // namespace

void NonlinSlew::resetState() {
    for (int i = 0; i < lastSampleBuf.size(); i++) {
        lastSampleBuf[i] = 0.0f;
        emaBuf[i] = 0.0f;    
    }
}

void NonlinSlew::prepare(juce::dsp::ProcessSpec& spec) {
    alphaParam.prepare(spec);
    slewSpeed.prepare(spec);
    directionality.prepare(spec);
    lastMode = -1;
    lastSampleBuf.resize(spec.numChannels);
    emaBuf.resize(spec.numChannels);

    resetState();

    sampleRateMultiplier = spec.sampleRate / 44100.0f;
    sampleRateMultInv = 44100.0f / spec.sampleRate;
}

void NonlinSlew::processBlock(juce::dsp::AudioBlock<float> &block) {
    alphaParam.update();
    slewSpeed.update();
    directionality.update();

    int mode = type->get();
    if (mode != lastMode) {
        resetState();
        lastMode = mode;
    }
    
    for (int ch = 0; ch < block.getNumChannels(); ch++) {
        float* data = block.getChannelPointer(ch);
        float& lastSample = lastSampleBuf[ch];
        float& ema = emaBuf[ch];

        switch (mode) {
            case 0: { // really basic min max slew
                for (int i = 0; i < block.getNumSamples(); i++) {
                    const float in = data[i];

                    float delta = in - lastSample;
            
                    float alpha = safePow(alphaParam.getNextValue(ch), 3.0f);

                    float rawSlewSpeed = slewSpeed.getNextValue(ch);
                    float slewSpeedVal = rawSlewSpeed * rawSlewSpeed * rawSlewSpeed * rawSlewSpeed;
                    float bend = directionality.getNextValue(ch);
                    
                    float jumpDist = juce::jmax(juce::jmin(delta, slewSpeedVal), -slewSpeedVal);
                    
                    lastSample = (1.0f - alpha) * in + alpha * (lastSample + jumpDist * sampleRateMultInv);
                    
                    if (!std::isfinite(lastSample)) lastSample = 0.0f;
                    
                    float gainCompensation = 2.0f - rawSlewSpeed;
                    data[i] = lastSample * gainCompensation;
                }
                break;
            }
            case 1: {
                // tanh based jumps
                for (int i = 0; i < block.getNumSamples(); i++) {
                    const float in = data[i];

                    float delta = in - lastSample;

                    int sign = sgn(delta);
            
                    float alpha = safePow(alphaParam.getNextValue(ch), 0.3f);
                    float slewSpeedVal = slewSpeed.getNextValue(ch);
                    float slewScaled = safePow(std::fabs(slewSpeedVal), 3.0f) * (slewSpeedVal < 0.0f ? -1.0f : 1.0f);
                    float bend = directionality.getNextValue(ch) * 4.0f;
            
                    float jumpDist = (tanh(delta * slewScaled * 50.0f * sampleRateMultiplier + bend * sampleRateMultiplier) - tanh(bend * sampleRateMultiplier)) * slewScaled * sampleRateMultInv;
            
                    lastSample = (1.0f - alpha) * in + alpha * std::tanh((lastSample + sign * delta * jumpDist) * sampleRateMultInv) * sampleRateMultiplier;
                    
                    if (!std::isfinite(lastSample)) lastSample = 0.0f;
                    
                    float gainCompensation = 2.0f - slewSpeedVal;
                    data[i] = lastSample * gainCompensation;
                }
            
                break;
            }
            case 2: {
                // gaussian / strange attractor?
                for (int i = 0; i < block.getNumSamples(); i++) {
                    const float in = data[i];
            
                    float delta = in - lastSample;

                    delta *= sampleRateMultiplier;
                    
                    float drive = safePow(alphaParam.getNextValue(0), 3.0f);
                    float alpha = drive * 50.0f;
                    float ema_a = drive;
                    float slew_bias = slewSpeed.getNextValue(0);
                    float bend = directionality.getNextValue(0);
    
                    float b = (delta + ema * bend);
                    float jumpDist = exp(-alpha * b * b) * slew_bias;
            
                    lastSample = std::tanh(lastSample + delta * jumpDist * sampleRateMultInv);
            
                    if (!std::isfinite(lastSample)) lastSample = 0.0f;
    
                    ema = ema_a * lastSample + (1.0f - ema_a) * ema;
                    if (!std::isfinite(ema)) ema = 0.0f;
    
                    float gainCompensation = drive + 1.0f;
                    data[i] = lastSample * gainCompensation;
                }
                
                break;
            }
    
    
    
        }

    }
}