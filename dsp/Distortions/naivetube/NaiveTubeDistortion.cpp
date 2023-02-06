#include "NaiveTubeDistortion.h"
#include <JuceHeader.h>

NaiveTubeDistortion::NaiveTubeDistortion(juce::AudioProcessorValueTreeState &state)
:
treeStateRef(state)
{
    // --- individual parameters
	preParameters.inputLevel_dB = 0.7;		///< input level in dB
	preParameters.saturation = 1.3;		///< input level in dB
	preParameters.asymmetry = 0.0;			///< input level in dB
	preParameters.outputLevel_dB = 0.0;	///< input level in dB

	// --- shelving filter params
	preParameters.lowShelf_fc = 100.0;			///< LSF shelf frequency
	preParameters.lowShelfBoostCut_dB = 0.0;	///< LSF shelf gain/cut
	preParameters.highShelf_fc = 5000.0;			///< HSF shelf frequency
	preParameters.highShelfBoostCut_dB = -0.3;	///< HSF shelf frequency

    knobValue = dynamic_cast<juce::AudioParameterFloat *>(treeStateRef.getParameter("knobID8"));
	jassert(knobValue); // makes sure it exists
}

NaiveTubeDistortion::~NaiveTubeDistortion()
{
}

void NaiveTubeDistortion::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    preL.reset(sampleRate);
    preL.setParameters(preParameters);
    preR.reset(sampleRate);
    preR.setParameters(preParameters);
}

void NaiveTubeDistortion::processBlock(juce::AudioBuffer<float>& buffer)
{
    float knobAmount = knobValue->get();
    if (preParameters.saturation != knobAmount) {
        preParameters.saturation = knobAmount * 6;
        preL.setParameters(preParameters);
        preR.setParameters(preParameters);
    }


    auto leftData = buffer.getWritePointer(0);
    auto rightData = buffer.getWritePointer(1);

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        leftData[sample] = preL.processAudioSample(leftData[sample]);
        rightData[sample] = preR.processAudioSample(rightData[sample]);
    }
}