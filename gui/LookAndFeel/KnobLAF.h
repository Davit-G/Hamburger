#pragma once
#include "../Utils/HamburgerFonts.h"


class KnobLAF : public juce::LookAndFeel_V4
{
public:
    KnobLAF(juce::Colour color = juce::Colour::fromRGB(50, 255, 205)) : knobColour(color)
    {
        setColour(Slider::rotarySliderOutlineColourId, juce::Colours::black);
        setColour(Slider::rotarySliderFillColourId, juce::Colours::white);
        setColour(Slider::thumbColourId, juce::Colours::whitesmoke);
        setColour(Slider::rotarySliderFillColourId, color);

        // dont use setsizeandstyle, it will remove the typeface info for windows specifically (weird bug)

        #if JUCE_WINDOWS
            questrialFont12->setHeight(18.0f);
            questrialFont14->setHeight(20.0f);
            questrialFont16->setHeight(22.0f);
        #else
            questrialFont12->setHeight(12.0f);
            questrialFont14->setHeight(14.0f);
            questrialFont16->setHeight(16.0f);
        #endif
    }

    ~KnobLAF()
    {
    }

    static Font getTheFont(int fontSize = 16)
    {
        return *questrialFont12;
    }

    Font getLabelFont(Label &label) override
    {
        return *questrialFont14;
    }

    Font getComboBoxFont(ComboBox &box) override
    {
        return *questrialFont14;
    }

    Font getPopupMenuFont() override
    {
        return *questrialFont16;
    }

    

    void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height, float sliderPos,
                          const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider &slider) override
    {

        // auto outline = slider.findColour(Slider::rotarySliderOutlineColourId);
        // auto fill = slider.findColour(Slider::rotarySliderFillColourId);
        auto thumb = slider.findColour(Slider::thumbColourId);

        auto bounds = Rectangle<int>(x, y, width, height).toFloat().reduced(5.0f);

        auto radius = jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        auto lineW = jmin(8.0f, radius * 0.5f);
        auto arcRadius = radius - lineW * 0.5f;

        // g.setColour(outline);

        // auto size = std::min(width, height);

        // the knob background
        // g.fillEllipse(Rectangle<float>(size, size).reduced(5.0f).withCentre(bounds.getCentre()));

        // some circles or something
        // g.setColour(knobColour);

        // g.drawEllipse(Rectangle<float>(size, size).reduced(7.0f).withCentre(bounds.getCentre()), 1.0f);
        // g.drawEllipse(Rectangle<float>(size, size).reduced(12.0f).withCentre(bounds.getCentre()), 2.0f);
        // g.drawEllipse(Rectangle<float>(size, size).reduced(20.0f).withCentre(bounds.getCentre()), 4.0f);

        // the marker for where the knob is
        // capsule shaped, so maybe thick line going outwards, with rounded caps at the end
        Line<float> marker;

        float xOffset = std::sin(toAngle) * arcRadius;
        float yOffset = -std::cos(toAngle) * arcRadius;

        marker.setStart(xOffset * 0.8f + bounds.getCentreX(), yOffset * 0.8f + bounds.getCentreY());
        marker.setEnd(xOffset + bounds.getCentreX(), yOffset + bounds.getCentreY());

        Path p;
        p.addLineSegment(marker, radius * 0.08f);
        g.setColour(thumb);
        g.strokePath(p, PathStrokeType(radius * 0.08f, PathStrokeType::JointStyle::curved, PathStrokeType::EndCapStyle::rounded));
    }

private:
    juce::Colour knobColour;
    
    static const std::unique_ptr<Font> questrialFont12;
    static const std::unique_ptr<Font> questrialFont14;
    static const std::unique_ptr<Font> questrialFont16;
};

const std::unique_ptr<juce::Font, std::default_delete<juce::Font>> KnobLAF::questrialFont12 = std::make_unique<juce::Font>(HamburgerFonts::questrialTypeface);
const std::unique_ptr<juce::Font, std::default_delete<juce::Font>> KnobLAF::questrialFont14 = std::make_unique<juce::Font>(HamburgerFonts::questrialTypeface);
const std::unique_ptr<juce::Font, std::default_delete<juce::Font>> KnobLAF::questrialFont16 = std::make_unique<juce::Font>(HamburgerFonts::questrialTypeface);
