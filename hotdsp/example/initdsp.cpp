// a starting point: drive into tanh with a bias. params are 0 drive (dB), then A to F (0 to 1) as laid out in the DLL box
#include "HotDsp.h"

#include <cmath>
#include <juce_dsp/juce_dsp.h> // works too, juce_dsp and audio_basics are linked in. costs ~0.4s a rebuild

#include "../dsp/EnvelopeFollower.h"

inline float omega (float x) { const float w = x > 20.0f ? x : std::log1p (std::exp (std::max (x, -80.0f))), r = x - w - std::log (w), a = 1.0f + w, b = a * (a + r * (2.0f / 3.0f)); return w * (1.0f + r / a * (b - 0.5f * r) / (b - r)); }

class Example : public HotDsp
{
public:
    Example() {}
    void prepare (const HotDspSpec& spec) override
    {
        juce::dsp::ProcessSpec juceSpec;
        juceSpec.maximumBlockSize = spec.maxSamples;
        juceSpec.numChannels = spec.numChannels;
        juceSpec.sampleRate = spec.sampleRate;

        sampleRate = spec.sampleRate;

        envFollower[0].prepare(juceSpec);
        envFollower[1].prepare(juceSpec);

        envFollowerSecond[0].prepare(juceSpec);
        envFollowerSecond[1].prepare(juceSpec);
        
    }

    void process (float* const* channels, int numChannels, int numSamples,
                  const float* const* params, int numParams) override
    {
        if (numParams < 2)
            return;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* x = channels[ch];

            for (int n = 0; n < numSamples; ++n)
            {
                const auto gain = std::pow (10.0f, params[0][n] / 20.0f);
                const auto bias = params[1][n];
                const auto envStrength = params[2][n] * 10.0f;
                const auto timingScale = params[3][n] * 2.0f;
                const auto autoCorrectionScale = params[4][n];

                const auto autoReduction = timingScale * fmax(0.01f, 1.0f - abs(env[ch]) * autoCorrectionScale * 0.5);

                envFollower[ch].setAttackTime(60.0f * timingScale);
                envFollower[ch].setReleaseTime(200.0f * timingScale);
                envFollowerSecond[ch].setAttackTime(5.0f * timingScale * autoReduction);
                envFollowerSecond[ch].setReleaseTime(50.0f * timingScale * autoReduction);
                
                env[ch] = envFollower[ch].processSample(omega(x[n] * gain)) * envStrength;
                x[n] = std::tanh (x[n] * gain - env[ch] * bias + env2[ch] * bias) - std::tanh (env[ch] * bias);    
                env2[ch] = envFollowerSecond[ch].processSample(-x[n] * gain) * envStrength;
            }
        }
    }

private:
    double sampleRate = 44100.0;

    float env[2], env2[2];

    EnvelopeFollower envFollower[2] {false, false };
    EnvelopeFollower envFollowerSecond[2] {false, false };
};

HOTDSP_EXPORT (Example)
