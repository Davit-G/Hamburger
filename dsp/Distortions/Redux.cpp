/*
  ==============================================================================

    Redux.cpp
    Created: 8 Dec 2021 5:01:30pm
    Author:  DavZ

  ==============================================================================
*/

#include "Redux.h"
#include <JuceHeader.h>


//==============================================================================
Redux::Redux(juce::AudioParameterFloat *downsampling, juce::AudioParameterFloat *bitReduction)
{
	// In your constructor, you should add any child components, and
	// initialise any special settings that your component needs.
	this->downsampling = downsampling;
	this->bitReduction = bitReduction;
}

Redux::~Redux()
{
}


void Redux::prepareToPlay(double sampleRate, int samplesPerBlock) {

}

void Redux::processBlock(dsp::AudioBlock<float>& block) {
	
	auto rightDryData = block.getChannelPointer(1);
	auto leftDryData = block.getChannelPointer(0);

	float reduxStereoValue = 0.0f;

	float downsamplingValue = downsampling->get();
	float bitReductionValue = bitReduction->get();

	if (downsamplingValue == 0.0f) {
		downsamplingValue = 0.0001f;
	}

	for (int sample = 0; sample < block.getNumSamples(); sample++) {

		auto x = rightDryData[sample];
		float posValues = powf(2, bitReductionValue);
		x = fmodf(x, 1 / posValues);  //bit reduction process
		//sample and hold process
		if (floor(fmodf(sample, downsamplingValue - reduxStereoValue)) == 0) {
			x = rightDryData[sample] - x;
		}
		else {
			x = rightDryData[sample - int(floor(fmodf(sample, downsamplingValue - reduxStereoValue)))];
		}

		rightDryData[sample] = x;


		x = leftDryData[sample];
		posValues = powf(2, bitReductionValue); //all the possible vertical steps
		x = fmodf(x, 1 / posValues);  //bit reduction process
		//sample and hold process
		if (floor(fmodf(sample, downsamplingValue + reduxStereoValue)) == 0) {
			x = leftDryData[sample] - x;
		}
		else {
			x = leftDryData[sample - int(floor(fmodf(sample, downsamplingValue + reduxStereoValue)))];
		}

		leftDryData[sample] = x;
	}
}