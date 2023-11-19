#pragma once

#include <JuceHeader.h>
#include "../Knob.h"

class Panel : public juce::Component
{
public:
    Panel(AudioPluginAudioProcessor &p, juce::String theName) : processor(p), name(theName) {
        setName(theName);
    }

    virtual ~Panel() = default;

protected:
    AudioPluginAudioProcessor &processor;

    juce::String name;
};