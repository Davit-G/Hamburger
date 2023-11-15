#pragma once

#include <JuceHeader.h>

class SaturationLookAndFeel : public juce::LookAndFeel_V4 
{
public:
    SaturationLookAndFeel(int pngSize, juce::Image knobImage, juce::Image shadowImage)  {
        setColour(juce::Slider::thumbColourId, juce::Colours::orange);
        size = pngSize;

        this->knobImage = knobImage;
        this->shadowImage = shadowImage;
    };

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider&) override
    {
        auto radius = (float) juce::jmin (width / 2, height / 2) - 4;
        auto centreX = (float) x + (float) width  * 0.5f;
        auto centreY = (float) y + (float) height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        float displacement3d = 0.0f;

        // pick from 0 to 128
        int imageIndex = (int) (sliderPos * 128.f);

        // get the image from the array
        imageSlice = knobImage.getClippedImage (juce::Rectangle<int> (imageIndex * size, 0, size, size));

        // preserve alpha
        g.setColour (juce::Colours::white.withAlpha (1.0f));

        int shadowMargin = 2;

        g.drawImage(shadowImage, -shadowMargin, -shadowMargin, width + shadowMargin*2, height + shadowMargin*2, 0, 0, shadowImage.getWidth(), shadowImage.getHeight(), false);

        // draw the image with alpha
        g.drawImage (imageSlice, rx, ry, rw, rw, 0, 0, size, size, false);

        // debugging square bounding box
        // g.setColour(juce::Colours::red);
        // g.drawRect(x, y, width, height, 2);
    }
private:
    int size;

    juce::Image knobImage;
    juce::Image shadowImage;

    juce::Image imageSlice;
};