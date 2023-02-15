
#include <JuceHeader.h>

class LightButton : public juce::Button {
    // this class is a button that renders either one of two images
    // depending on whether it is toggled or not

public:
    LightButton(AudioPluginAudioProcessor &p, juce::String attachmentID, const juce::Image& offImage, const juce::Image& onImage) 
    : processorRef(p), onImage(onImage), offImage(offImage), Button(attachmentID)
    {
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processorRef.treeState, attachmentID, *this);

        setClickingTogglesState(true);
    }

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        if (getToggleState()) {
            g.drawImage(onImage, 0, 0, getWidth(), getHeight(), 0, 0, onImage.getWidth(), onImage.getHeight(), false);
        }
        else {
            g.drawImage(offImage, 0, 0, getWidth(), getHeight(), 0, 0, onImage.getWidth(), onImage.getHeight(), false);
        }
    }

private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment = nullptr;

    AudioPluginAudioProcessor &processorRef;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> buttonValue;

    juce::Image onImage;
    juce::Image offImage;
};

