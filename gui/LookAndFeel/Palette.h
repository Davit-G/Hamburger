#pragma once

class Palette
{
public:
    const static std::vector<juce::Colour> colours;

    static void setKnobColoursOfComponent(juce::Component *component, juce::Colour color)
    {
        component->setColour(Slider::rotarySliderOutlineColourId, juce::Colours::black);
        component->setColour(Slider::rotarySliderFillColourId, juce::Colours::white);
        component->setColour(Slider::thumbColourId, juce::Colours::whitesmoke);
        component->setColour(Slider::rotarySliderFillColourId, color);
    }

    static void setKnobColoursOfComponent(juce::Component *component, int index)
    {
        setKnobColoursOfComponent(component, colours[index]);
    }
};

const std::vector<juce::Colour> Palette::colours = {
    juce::Colour::fromRGB(255, 89, 119),
    juce::Colour::fromRGB(255, 218, 69),
    juce::Colour::fromRGB(255, 146, 75),
    juce::Colour::fromRGB(0, 229, 138),
    juce::Colour::fromRGB(255, 46, 151),
    juce::Colour::fromRGB(78, 247, 255),
    juce::Colour::fromRGB(59, 120, 255)
};

