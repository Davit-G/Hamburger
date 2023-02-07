
#include <JuceHeader.h>

class Gain
{
public:
    Gain()
    {
        // initialise gain to 1.0
        gain = 1.0;
    }
    ~Gain() {}
    void setGain(float newGain)
    {
        gain = newGain;
    }
    float getGain()
    {
        return gain;
    }
    float process(float input)
    {
        return input * gain;
    }

    float processBlock(juce::AudioBuffer<float> &buffer)
    {
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                buffer.setSample(channel, sample, buffer.getSample(channel, sample) * gain);
            }
        }
    }

private:
    float gain;
};