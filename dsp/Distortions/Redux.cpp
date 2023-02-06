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
Redux::Redux()
{
	// In your constructor, you should add any child components, and
	// initialise any special settings that your component needs.

}

Redux::~Redux()
{
}


void Redux::prepareToPlay(double sampleRate, int samplesPerBlock) {

}

void Redux::processBlock(float bitReductionValue, float downSampleValue, float reduxStereoValue, AudioBuffer<float>& dryBuffer) {
	
	auto rightDryData = dryBuffer.getWritePointer(1);
	auto leftDryData = dryBuffer.getWritePointer(0);

	for (int sample = 0; sample < dryBuffer.getNumSamples(); sample++) {

		auto x = rightDryData[sample];
		float posValues = powf(2, bitReductionValue);
		x = fmodf(x, 1 / posValues);  //bit reduction process
		//sample and hold process
		if (floor(fmodf(sample, downSampleValue - reduxStereoValue)) == 0) {
			x = rightDryData[sample] - x;
		}
		else {
			x = rightDryData[sample - int(floor(fmodf(sample, downSampleValue - reduxStereoValue)))];
		}

		rightDryData[sample] = x;


		x = leftDryData[sample];
		posValues = powf(2, bitReductionValue); //all the possible vertical steps
		x = fmodf(x, 1 / posValues);  //bit reduction process
		//sample and hold process
		if (floor(fmodf(sample, downSampleValue + reduxStereoValue)) == 0) {
			x = leftDryData[sample] - x;
		}
		else {
			x = leftDryData[sample - int(floor(fmodf(sample, downSampleValue + reduxStereoValue)))];
		}

		leftDryData[sample] = x;


	}
}