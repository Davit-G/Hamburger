#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

class Palette
{
public:
    const static std::vector<juce::Colour> colours;

    static void setKnobColoursOfComponent(juce::Component *component, juce::Colour color)
    {
        component->setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::black);
        component->setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::white);
        component->setColour(juce::Slider::thumbColourId, juce::Colours::whitesmoke);
        component->setColour(juce::Slider::rotarySliderFillColourId, color);
    }

    static void setKnobColoursOfComponent(juce::Component *component, int index)
    {
        setKnobColoursOfComponent(component, colours[index]);
    }
};