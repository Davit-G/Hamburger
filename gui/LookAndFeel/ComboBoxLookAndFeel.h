#pragma once
#include "../Utils/HamburgerFonts.h"


class ComboBoxLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ComboBoxLookAndFeel()
    {

        setColour(juce::ComboBox::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::ComboBox::ColourIds::buttonColourId, juce::Colours::transparentBlack);
        setColour(juce::ComboBox::ColourIds::outlineColourId, juce::Colours::transparentBlack);
        // setColour(juce::ComboBox::ColourIds::focusedOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::ComboBox::ColourIds::textColourId, juce::Colours::white);
        setColour(juce::PopupMenu::ColourIds::backgroundColourId, juce::Colours::black);
        setColour(juce::PopupMenu::ColourIds::textColourId, juce::Colours::white);
        setColour(juce::PopupMenu::ColourIds::highlightedBackgroundColourId, juce::Colour::fromHSV(0.0f, 0.0f, 0.2f, 1.0f));
        setColour(juce::PopupMenu::ColourIds::highlightedTextColourId, juce::Colours::white);
        setColour(juce::ComboBox::arrowColourId, juce::Colours::white);
    
        #if JUCE_WINDOWS
            questrialFont->setHeight(20);
            quicksandFont->setHeight(24);
        #else
            questrialFont->setHeight(14);
            quicksandFont->setHeight(18);
        #endif

    }

    Font getComboBoxFont()
    {
        
        return *questrialFont;
    }

    Font getPopupMenuFont()
    {
        
        return *questrialFont;
    }

    Font getLabelFont(Label &label) override
    {
        return *questrialFont;
    }

    Font getAlertWindowFont() override
    {
        return *questrialFont;
    }

    Font getAlertWindowMessageFont() override
    {
        return *questrialFont;
    }

    Font getAlertWindowTitleFont() override
    {
        return *quicksandFont;
    }


    void drawComboBox(Graphics &g, int width, int height, bool,
                      int, int, int, int, ComboBox &box)
    {
        auto cornerSize = box.findParentComponentOfClass<ChoicePropertyComponent>() != nullptr ? 0.0f : 3.0f;
        Rectangle<int> boxBounds(0, 0, width, height);

        g.setColour(box.findColour(ComboBox::backgroundColourId));
        g.fillRoundedRectangle(boxBounds.toFloat(), cornerSize);

        g.setColour(box.findColour(ComboBox::outlineColourId));
        g.drawRoundedRectangle(boxBounds.toFloat().reduced(0.5f, 0.5f), cornerSize, 1.0f);

        Rectangle<int> arrowZone(5, 0, 20, height);
        Path path;
        path.startNewSubPath((float)arrowZone.getX() + 4.0f, (float)arrowZone.getCentreY() - 2.0f);
        path.lineTo((float)arrowZone.getCentreX(), (float)arrowZone.getCentreY() + 3.0f);
        path.lineTo((float)arrowZone.getRight() - 4.0f, (float)arrowZone.getCentreY() - 2.0f);

        g.setColour(box.findColour(ComboBox::arrowColourId).withAlpha((box.isEnabled() ? 0.9f : 0.2f)));
        g.strokePath(path, PathStrokeType(2.0f, juce::PathStrokeType::JointStyle::curved, juce::PathStrokeType::EndCapStyle::rounded));
    }

    void positionComboBoxText(ComboBox &box, Label &label)
    {
        label.setBounds(30, 1,
                        box.getWidth() - 30,
                        box.getHeight() - 2);

        // label.setJustificationType(juce::Justification::centredRight);

        label.setFont(getComboBoxFont());
    }

    void drawPopupMenuItem(Graphics &g, const Rectangle<int> &area,
                           const bool isSeparator, const bool isActive,
                           const bool isHighlighted, const bool isTicked,
                           const bool hasSubMenu, const String &text,
                           const String &shortcutKeyText,
                           const Drawable *icon, const Colour *const textColourToUse)
    {
        if (isSeparator)
        {
            auto r = area.reduced(5, 0);
            r.removeFromTop(roundToInt(((float)r.getHeight() * 0.5f) - 0.5f));

            g.setColour(findColour(PopupMenu::textColourId).withAlpha(0.3f));
            g.fillRect(r.removeFromTop(1));
        }
        else
        {
            auto textColour = (textColourToUse == nullptr ? findColour(PopupMenu::textColourId)
                                                          : *textColourToUse);

            auto r = area.reduced(1);

            if (isHighlighted && isActive)
            {
                g.setColour(findColour(PopupMenu::highlightedBackgroundColourId));
                g.fillRect(r);

                g.setColour(findColour(PopupMenu::highlightedTextColourId));
            }
            else
            {
                g.setColour(textColour.withMultipliedAlpha(isActive ? 1.0f : 0.5f));
            }

            r.reduce(jmin(5, area.getWidth() / 20), 0);

            auto font = getPopupMenuFont();

            auto maxFontHeight = (float)r.getHeight() / 1.3f;

            if (font.getHeight() > maxFontHeight)
                font.setHeight(maxFontHeight);

            g.setFont(font);

            auto iconArea = r.removeFromLeft(roundToInt(maxFontHeight)).toFloat();

            if (icon != nullptr)
            {
                icon->drawWithin(g, iconArea, RectanglePlacement::centred | RectanglePlacement::onlyReduceInSize, 1.0f);
                r.removeFromLeft(roundToInt(maxFontHeight * 0.5f));
            }
            else if (isTicked)
            {
                // auto tick = getTickShape(1.0f);
                // g.fillPath(tick, tick.getTransformToScaleToFit(iconArea.reduced(iconArea.getWidth() / 5, 0).toFloat(), true));
                g.drawImage(juce::ImageCache::getFromMemory(BinaryData::tick_png, BinaryData::tick_pngSize), iconArea.reduced(iconArea.getWidth() / 5, iconArea.getHeight() / 5).toFloat());
            }

            if (hasSubMenu)
            {
                auto arrowH = 0.6f * getPopupMenuFont().getAscent();

                auto x = static_cast<float>(r.removeFromRight((int)arrowH).getX());
                auto halfH = static_cast<float>(r.getCentreY());

                Path path;
                path.startNewSubPath(x, halfH - arrowH * 0.5f);
                path.lineTo(x + arrowH * 0.6f, halfH);
                path.lineTo(x, halfH + arrowH * 0.5f);

                g.strokePath(path, PathStrokeType(2.0f));
            }

            r.removeFromRight(3);
            g.drawFittedText(text, r, Justification::centredLeft, 1);

            if (shortcutKeyText.isNotEmpty())
            {
                auto f2 = font;
                f2.setHeight(f2.getHeight() * 0.75f);
                f2.setHorizontalScale(0.95f);
                g.setFont(f2);

                g.drawText(shortcutKeyText, r, Justification::centredRight, true);
            }
        }
    }

private:

    static const std::unique_ptr<Font> questrialFont;
    static const std::unique_ptr<Font> quicksandFont;
};

const std::unique_ptr<juce::Font, std::default_delete<juce::Font>> ComboBoxLookAndFeel::questrialFont = std::make_unique<juce::Font>(HamburgerFonts::questrialTypeface);
const std::unique_ptr<juce::Font, std::default_delete<juce::Font>> ComboBoxLookAndFeel::quicksandFont = std::make_unique<juce::Font>(HamburgerFonts::quicksandTypeface);
