#pragma once

#include <JuceHeader.h>
#include "../Knob.h"

class Panel : public juce::Component
{
public:
    Panel(AudioPluginAudioProcessor &p, juce::String name) : processor(p), name(name) {
        setName(name);
    }

    virtual ~Panel() = default;

protected:
    AudioPluginAudioProcessor &processor;

    juce::String name;
};