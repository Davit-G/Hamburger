#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

// custom toggle button
class BoxToggle : public juce::ToggleButton
{
public:
    explicit BoxToggle (const juce::String& text)
    {
        setButtonText (text);

        label.setText (text, juce::dontSendNotification);
        label.setColour (juce::Label::textColourId, juce::Colours::white);
        label.setBorderSize ({});
        label.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (label);
    }

    void paintButton (juce::Graphics& g, bool, bool) override
    {
        const auto box = juce::Rectangle<float> (boxSize, boxSize).withCentre ({ boxSize * 0.5f, (float) getHeight() * 0.5f });

        g.setColour (findColour (juce::Slider::rotarySliderFillColourId));
        g.drawRect (box, 1.0f);

        if (getToggleState())
            g.fillRect (box.reduced (2.0f));
    }

    void resized() override {
        label.setBounds (getLocalBounds().withTrimmedLeft ((int) boxSize + gap));
    }

private:
    static constexpr float boxSize = 14.0f;
    static constexpr int gap = 10;

    juce::Label label;
};
