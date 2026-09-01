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

    static void setLabelFontScale(juce::Label &label, float scale)
    {
        label.getProperties().set("fontScale", scale);
        label.repaint();
    }

    juce::Font getComboBoxFont(juce::ComboBox &box) override;

    juce::Font getPopupMenuFont() override;

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
    
    static constexpr int tooltipPaddingX = 24;
    static constexpr int tooltipPaddingY = 16;
    
    juce::TextLayout createTooltipLayout(const juce::String &text) const
    {
        const float maxTooltipWidth = 300.0f; // max width before wrapping text

        juce::Font font = *quicksandFont;

        juce::AttributedString s;
        s.setJustification(juce::Justification::centred);
        s.append(text, font, juce::Colours::white);

        juce::TextLayout tl;
        tl.createLayout(s, maxTooltipWidth);
        return tl;
    }

    juce::Rectangle<int> getTooltipBounds(const juce::String &tipText, juce::Point<int> screenPos,
                                          juce::Rectangle<int> parentArea) override
    {
        auto layout = createTooltipLayout(tipText);

        auto w = (int)std::ceil(layout.getWidth()) + tooltipPaddingX;
        auto h = (int)std::ceil(layout.getHeight()) + tooltipPaddingY;

        return juce::Rectangle<int>(screenPos.x > parentArea.getCentreX() ? screenPos.x - (w + 12) : screenPos.x + 24,
                                    screenPos.y > parentArea.getCentreY() ? screenPos.y - (h + 6) : screenPos.y + 6,
                                    w, h)
            .constrainedWithin(parentArea);
    }

    void drawTooltip(juce::Graphics &g, const juce::String &text, int width, int height) override
    {
        auto bounds = juce::Rectangle<int>(width, height);

        // bg
        g.setColour(juce::Colours::black);
        g.fillRoundedRectangle(bounds.toFloat(), 5.0f);

        // outline
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f, 0.5f), 5.0f, 2.0f);

        auto layout = createTooltipLayout(text);

        float textX = (width - layout.getWidth()) / 2.0f;
        float textY = (height - layout.getHeight()) / 2.0f;

        layout.draw(g, juce::Rectangle<float>(textX, textY, layout.getWidth(), layout.getHeight()));
    }

private:
    juce::Colour knobColour;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HamburgerLAF);
};