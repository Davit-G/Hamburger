
#include "Valve.h"
#include "juce_core/juce_core.h"
#include "juce_dsp/juce_dsp.h"
#include "juce_audio_processors/juce_audio_processors.h"

#include <melatonin_perfetto/melatonin_perfetto.h>

#include "../../../utils/Params.h"
#include "../../SmoothParam.h"

class Amp
{
public:
    Amp(juce::AudioProcessorValueTreeState &treeState) : 
        tubeTone(treeState, ParamIDs::tubeTone),
        drive(treeState, ParamIDs::tubeAmount),
        bias(treeState, ParamIDs::tubeBias)
        {}

    ~Amp() {}

    void calcCoefficients();
    void calcCoefficientsPerSample();
    void calcCoefficients2();

    void prepare(juce::dsp::ProcessSpec& spec);
    void processBlock(juce::dsp::AudioBlock<float> &block);

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