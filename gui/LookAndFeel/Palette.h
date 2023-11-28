#pragma once

class Palette
{
public:
    const static std::vector<juce::Colour> colours;
};

const std::vector<juce::Colour> Palette::colours = {
    juce::Colour::fromRGB(255, 89, 119),
    juce::Colour::fromRGB(255, 218, 69),
    juce::Colour::fromRGB(255, 146, 75),
    juce::Colour::fromRGB(0, 229, 138),
    juce::Colour::fromRGB(255, 46, 151),
    juce::Colour::fromRGB(78, 247, 255),
    juce::Colour::fromRGB(59, 120, 255)};

