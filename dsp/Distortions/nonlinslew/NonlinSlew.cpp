#include "NonlinSlew.h"

#include <algorithm>
#include <cmath>

void NonlinSlew::resetState() {
    std::fill(lastSampleBuf.begin(), lastSampleBuf.end(), 0.0f);
    std::fill(emaBuf.begin(), emaBuf.end(), 0.0f);
}

void NonlinSlew::prepare(juce::dsp::ProcessSpec& spec) {
    alphaParam.prepare(spec);
    slewSpeed.prepare(spec);
    directionality.prepare(spec);
    lastMode = -1;
    lastSampleBuf.resize(spec.numChannels);
    emaBuf.resize(spec.numChannels);
    inPower.assign(spec.numChannels, 0.0f);
    outPower.assign(spec.numChannels, 0.0f);
    levelCoefficient = std::exp(-1.0f / (0.15f * (float) spec.sampleRate));
    sampleRate = (float) spec.sampleRate;

    resetState();
}

void NonlinSlew::processBlock(juce::dsp::AudioBlock<float> &block) {
    alphaParam.update();
    slewSpeed.update();
    directionality.update();

    const int mode = juce::roundToInt(type.get());

    if (mode != lastMode) {
        resetState();
        lastMode = mode;
    }

    for (int ch = 0; ch < (int) block.getNumChannels(); ch++) {
        float* data = block.getChannelPointer(ch);
        float& last = lastSampleBuf[ch];
        float& ema = emaBuf[ch];

        for (int i = 0; i < (int) block.getNumSamples(); i++) {
            const float driveGain = juce::Decibels::decibelsToGain(alphaParam.getNextValue(ch));
            const float limit = juce::MathConstants<float>::twoPi * slewSpeed.getNextValue(ch) * 0.25f / sampleRate;
            const float bias = directionality.getNextValue(ch);

            const float in = data[i];
            const float delta = in * driveGain - last;
            const float reach = limit * juce::jmax(0.05f, delta > 0.0f ? 1.0f + bias : 1.0f - bias);

            switch (mode) {
                case 0: // a hard limit
                    last += juce::jlimit(-reach, reach, delta);
                    break;

                case 1: // eased into the limit
                    last += reach * std::tanh(delta / reach);
                    break;

                // small steps pass and big ones are held back, saturating as it goes. it cuts off so steeply that its window
                // is wider, putting the cut in the middle of the tone's range
                default:
                    last = std::tanh(last + delta * std::exp(-juce::square((delta + ema * bias) / (reach * 8.0f))));
                    ema += juce::jmin(1.0f, limit) * (last - ema);
                    break;
            }

            if (!std::isfinite(last)) last = 0.0f;

            // how much slewing takes off depends on how fast and loud the audio is, so the output follows the input's level
            // over 150ms, by at most 12dB either way so a near silent setting isn't brought back up as noise
            const float out = last / driveGain;
            
            inPower[ch] = in * in + levelCoefficient * (inPower[ch] - in * in);
            outPower[ch] = out * out + levelCoefficient * (outPower[ch] - out * out);

            data[i] = out * juce::jlimit(0.25f, 4.0f, std::sqrt(inPower[ch] / (outPower[ch] + 1.0e-12f)));
        }
    }
}
