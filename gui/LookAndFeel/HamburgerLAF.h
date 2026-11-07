#pragma once

#include "juce_gui_basics/juce_gui_basics.h"
#include "BinaryData.h"
#include "Theme.h"

class HamburgerLAF : public juce::LookAndFeel_V4
{
public:
    HamburgerLAF(juce::Colour color = juce::Colour::fromRGB(50, 255, 205));

    // the colours JUCE's own widgets look up, like the dropdown menus
    void applyTheme();

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

    // a hand over everything that can be clicked, unless it's asked for a cursor of its own
    juce::MouseCursor getMouseCursorFor(juce::Component &component) override
    {
        const auto clickable = dynamic_cast<juce::ComboBox *>(&component) != nullptr || dynamic_cast<juce::Button *>(&component) != nullptr;

        if (clickable && component.getMouseCursor() == juce::MouseCursor::NormalCursor)
            return juce::MouseCursor::PointingHandCursor;

        return LookAndFeel_V4::getMouseCursorFor(component);
    }

    juce::Font getPopupMenuFont() override;

    void drawComboBox(juce::Graphics &g, int width, int height, bool,
                      int, int, int, int, juce::ComboBox &box) override;

    juce::Font getComboBoxFont();

    juce::Font getAlertWindowFont() override;

    juce::Font getAlertWindowMessageFont() override;

    juce::Font getAlertWindowTitleFont() override;

    void positionComboBoxText(juce::ComboBox &box, juce::Label &label) override;

    // the menus drawn like the tooltips and the scale readout, rounded
    void drawPopupMenuBackground(juce::Graphics &g, int width, int height) override
    {
        g.setColour(theme().popupBackground);
        g.fillRoundedRectangle(juce::Rectangle<int>(width, height).toFloat(), 5.0f);
    }

    // a thin line between a menu's columns, for menus whose box asks for one
    int getPopupMenuColumnSeparatorWidthWithOptions(const juce::PopupMenu::Options &options) override
    {
        const auto *target = options.getTargetComponent();
        return target != nullptr && target->getProperties()["columnDividers"] ? 9 : 0;
    }

    // juce draws one between every menu's columns, even with no width given to it
    void drawPopupMenuColumnSeparatorWithOptions(juce::Graphics &g, const juce::Rectangle<int> &bounds, const juce::PopupMenu::Options &) override
    {
        if (bounds.getWidth() <= 0)
            return;

        g.setColour(theme().divider);
        g.fillRect(bounds.withSizeKeepingCentre(1, bounds.getHeight() - 12));
    }

    // keeps the items, and the highlight under the mouse, inside the rounded corners
    int getPopupMenuBorderSize() override { return 5; }

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
        s.append(text, font, theme().popupText);

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

        g.setColour(theme().popupBackground);
        g.fillRoundedRectangle(bounds.toFloat(), 5.0f);

        auto layout = createTooltipLayout(text);

        float textX = (width - layout.getWidth()) / 2.0f;
        float textY = (height - layout.getHeight()) / 2.0f;

        layout.draw(g, juce::Rectangle<float>(textX, textY, layout.getWidth(), layout.getHeight()));
    }

private:
    juce::Colour knobColour;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HamburgerLAF);
};