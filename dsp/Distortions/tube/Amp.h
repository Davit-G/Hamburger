// heavily inspired by will pirkle
#include "Valve.h"

#include "../../SmoothParam.h"

class Amp
{
public:
    Amp(juce::AudioProcessorValueTreeState &treeState) : 
        tubeTone(treeState, "tubeTone"),
        drive(treeState, "tubeAmount"),
        bias(treeState, "bias")
        {}

    ~Amp() {}

    void calculateCoefficients();

    void prepare(dsp::ProcessSpec& spec);
    void processBlock(dsp::AudioBlock<float> &block);

    float inputGain = 0.5f;
    float driveGain = 1.0f;
    float tubeCompress = 1.0;
	float outputGain = 2.0f;

private:
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> inputHighPass;

    ClassAValve<float> triodes[4];

    SmoothParam tubeTone;
    SmoothParam drive;
    SmoothParam bias;

    float sampleRate = 44100.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Amp)
};