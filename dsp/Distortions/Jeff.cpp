/*
  ==============================================================================

    Jeff.cpp
    Created: 13 Jun 2021 5:57:58pm
    Author:  DavZ

  ==============================================================================
*/

#include "../../JuceLibraryCode/JuceHeader.h"
#include "Jeff.h"

//==============================================================================
Jeff::Jeff()
{
	// In your constructor, you should add any child components, and
	// initialise any special settings that your component needs.

}

Jeff::~Jeff()
{
}


void Jeff::prepareToPlay(double sampleRate, int samplesPerBlock) {
	smoothedInput.reset(44100, 0.07);
	smoothedInput.setCurrentAndTargetValue(0.0);
}

void Jeff::processBlock(float input, AudioBuffer<float>& dryBuffer) {
	smoothedInput.setTargetValue(input);
	auto rightDryData = dryBuffer.getWritePointer(1);
	auto leftDryData = dryBuffer.getWritePointer(0);

	for (int sample = 0; sample < dryBuffer.getNumSamples(); sample++) {
		float nextJeff = smoothedInput.getNextValue();

		auto x = rightDryData[sample];
		rightDryData[sample] = x + (x*nextJeff*sin(x * 2 * nextJeff * 20 * 3.14)) / 5;


		x = leftDryData[sample];
		leftDryData[sample] = x + (x*nextJeff*sin(x * 2 * nextJeff * 20 * 3.14)) / 5;

	}
}