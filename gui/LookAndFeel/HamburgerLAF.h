#pragma once

#include "juce_gui_basics/juce_gui_basics.h"
#include "BinaryData.h"

class HamburgerLAF : public juce::LookAndFeel_V4
{
public:
    HamburgerLAF(juce::Colour color = juce::Colour::fromRGB(50, 255, 205));

    const juce::Typeface::Ptr questrialTypeface = juce::Typeface::createSystemTypefaceFor(BinaryData::QuestrialRegular_ttf, BinaryData::QuestrialRegular_ttfSize);
    const juce::Typeface::Ptr quicksandTypeface = juce::Typeface::createSystemTypefaceFor(BinaryData::QuicksandBold_ttf, BinaryData::QuicksandBold_ttfSize);

    const std::unique_ptr<juce::Font> questrialFont = std::make_unique<juce::Font>(questrialTypeface);
    const std::unique_ptr<juce::Font> questrialFont12 = std::make_unique<juce::Font>(questrialTypeface);
    const std::unique_ptr<juce::Font> questrialFont14 = std::make_unique<juce::Font>(questrialTypeface);
    const std::unique_ptr<juce::Font> questrialFont16 = std::make_unique<juce::Font>(questrialTypeface);
    const std::unique_ptr<juce::Font> quicksandFont = std::make_unique<juce::Font>(quicksandTypeface);

    juce::Font getQuicksandFont();

    juce::Font getLabelFont(juce::Label &label) override;

    juce::Font getComboBoxFont(juce::ComboBox &box) override;

    juce::Font getPopupMenuFont() override;

    void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height, float sliderPos,
                          const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider &slider) override;

    void drawComboBox(juce::Graphics &g, int width, int height, bool,
                      int, int, int, int, juce::ComboBox &box) override;

    juce::Font getComboBoxFont();

    juce::Font getAlertWindowFont() override;

    juce::Font getAlertWindowMessageFont() override;

    juce::Font getAlertWindowTitleFont() override;

    void positionComboBoxText(juce::ComboBox &box, juce::Label &label) override;

    void drawPopupMenuItem(juce::Graphics &g, const juce::Rectangle<int> &area,
                           const bool isSeparator, const bool isActive,
                           const bool isHighlighted, const bool isTicked,
                           const bool hasSubMenu, const juce::String &text,
                           const juce::String &shortcutKeyText,
                           const juce::Drawable *icon, const juce::Colour *const textColourToUse) override;
private:
    juce::Colour knobColour;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HamburgerLAF);
};