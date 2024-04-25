#pragma once

#include "HamburgerLAF.h"

class KnobLAF : public HamburgerLAF
{
public:
    KnobLAF(juce::Colour color = juce::Colour::fromRGB(50, 255, 205)) : knobColour(color)
    {
        // call parent constructor
        HamburgerLAF::HamburgerLAF(color);

        setColour(Slider::rotarySliderOutlineColourId, juce::Colours::black);
        setColour(Slider::rotarySliderFillColourId, juce::Colours::white);
        setColour(Slider::thumbColourId, juce::Colours::whitesmoke);
        setColour(Slider::rotarySliderFillColourId, color);
        
    }

    ~KnobLAF()
    {
    }

private:
    juce::Colour knobColour;
};