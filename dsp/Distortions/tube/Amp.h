#include <JuceHeader.h>
#include "Valve.h"

#include "../../SmoothParam.h"

class Amp
{
public:
    Amp(juce::AudioProcessorValueTreeState &treeState) : 
        tubeTone(treeState, "tubeTone"),
        drive(treeState, "saturationAmount")
    {  
		calculateCoefficients();
    }

    ~Amp() {}

    void calculateCoefficients();
    void prepare(dsp::ProcessSpec& spec);
    void processBlock(dsp::AudioBlock<float> &block);

    float inputGain = 1.0f;
    float driveGain = 1.0f;
    float tubeCompress = 1.0;
	float outputGain = 1.0;

private:
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> inputHighPass;

    ClassAValve triodesL[4];
    ClassAValve triodesR[4];

    SmoothParam tubeTone;
    SmoothParam drive;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Amp)
};