#pragma once
#include <JuceHeader.h>


class KnobLAF : public juce::LookAndFeel_V4
{
public:
    KnobLAF(juce::Colour color = juce::Colour::fromRGB(50, 255, 205)) : knobColour(color) {
        setColour(Slider::rotarySliderOutlineColourId, juce::Colours::black);
        setColour(Slider::rotarySliderFillColourId, juce::Colours::white);
        setColour(Slider::thumbColourId, juce::Colours::whitesmoke);
    }

    static Font getTheFont(int fontSize = 16) {
        auto font = Font(Typeface::createSystemTypefaceFor(BinaryData::QuestrialRegular_ttf, BinaryData::QuestrialRegular_ttfSize));
        font.setSizeAndStyle(fontSize, font.getAvailableStyles()[0], 1, 0);

        auto styles = font.getAvailableStyles();
        return font;
    }

    Font getLabelFont(Label& label) override {
        return getTheFont(16);
    }

    Font getComboBoxFont(ComboBox& box) override
    {
        return getTheFont(16);
    }

    Font getPopupMenuFont() override
    {
        return getTheFont(32);
    }

    void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height, float sliderPos,
                          const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider &slider) override
    {
        auto outline = slider.findColour(Slider::rotarySliderOutlineColourId);
        auto fill = slider.findColour(Slider::rotarySliderFillColourId);
        auto thumb = slider.findColour(Slider::thumbColourId);

        auto bounds = Rectangle<int>(x, y, width, height).toFloat().reduced(5.0f);

        auto radius = jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        auto lineW = jmin(8.0f, radius * 0.5f);
        auto arcRadius = radius - lineW * 0.5f;

        g.setColour(outline);
        
        auto size = std::min(width, height);

        // the knob background
        g.fillEllipse(Rectangle<float>(size, size).reduced(5.0f).withCentre(bounds.getCentre()));

        // some circles or something
        g.setColour(knobColour);

        // g.drawEllipse(Rectangle<float>(size, size).reduced(7.0f).withCentre(bounds.getCentre()), 1.0f);
        g.drawEllipse(Rectangle<float>(size, size).reduced(12.0f).withCentre(bounds.getCentre()), 2.0f);
        g.drawEllipse(Rectangle<float>(size, size).reduced(20.0f).withCentre(bounds.getCentre()), 4.0f);

        
        // the marker for where the knob is
        // capsule shaped, so maybe thick line going outwards, with rounded caps at the end
        Line<float> marker;

        float xOffset = std::sin(toAngle) * arcRadius;
        float yOffset = -std::cos(toAngle) * arcRadius;

        marker.setStart(xOffset * 0.8 + bounds.getCentreX(), yOffset * 0.8 + bounds.getCentreY());
        marker.setEnd(xOffset + bounds.getCentreX(), yOffset + bounds.getCentreY());
        
        Path p;
        p.addLineSegment(marker, radius * 0.08f);
        g.setColour(thumb);
        g.strokePath(p, PathStrokeType(radius * 0.08f, PathStrokeType::JointStyle::curved, PathStrokeType::EndCapStyle::rounded));
        
    }

private:

    juce::Colour knobColour;
};