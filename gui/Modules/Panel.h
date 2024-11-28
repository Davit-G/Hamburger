#pragma once

#include "../LookAndFeel/Palette.h"


class Panel : public juce::Component
{
public:
    Panel(AudioPluginAudioProcessor &p, juce::String theName, juce::Colour color = juce::Colours::white) : name(theName) {
        setName(theName);

        Palette::setKnobColoursOfComponent(this, color);
    }

    virtual ~Panel() = default;

protected:

    juce::String name;
};