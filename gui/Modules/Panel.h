#pragma once

 

class Panel : public juce::Component
{
public:
    Panel(AudioPluginAudioProcessor &p, juce::String theName) : name(theName) {
        setName(theName);
    }

    virtual ~Panel() = default;

protected:

    juce::String name;
};