#pragma once
#include <JuceHeader.h>

class LightButton : public juce::Button {
    // this class is a button that renders either one of two images
    // depending on whether it is toggled or not

public:
    LightButton(AudioPluginAudioProcessor &p, const juce::Image& aoffImage, const juce::Image& aonImage) 
    : onImage(aonImage), offImage(aoffImage), Button("thing")
    {
        // attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processorRef.treeState, "attachmentID", *this);

        setClickingTogglesState(true);

        // setToggleState(ge, juce::dontSendNotification);
    }

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        juce::ignoreUnused(shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
        if (getToggleState()) {
            g.drawImage(onImage, 0, 0, getWidth(), getHeight(), 0, 0, onImage.getWidth(), onImage.getHeight(), false);
        }
        else {
            g.drawImage(offImage, 0, 0, getWidth(), getHeight(), 0, 0, onImage.getWidth(), onImage.getHeight(), false);
        }
    }

private:
    juce::Image onImage;
    juce::Image offImage;
};

