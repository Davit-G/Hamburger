#pragma once

#include <JuceHeader.h>

class ComboBoxLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ComboBoxLookAndFeel()
    {

        setColour(juce::ComboBox::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::ComboBox::ColourIds::buttonColourId, juce::Colours::transparentBlack);
        setColour(juce::ComboBox::ColourIds::outlineColourId, juce::Colours::transparentBlack);
        // setColour(juce::ComboBox::ColourIds::focusedOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::ComboBox::ColourIds::textColourId, juce::Colours::black);
        setColour(juce::PopupMenu::ColourIds::backgroundColourId, juce::Colours::white);
        setColour(juce::PopupMenu::ColourIds::textColourId, juce::Colours::black);
        setColour(juce::PopupMenu::ColourIds::highlightedBackgroundColourId, juce::Colour::fromRGBA(220, 220, 220, 255));
        setColour(juce::PopupMenu::ColourIds::highlightedTextColourId, juce::Colours::black);
        setColour(juce::ComboBox::arrowColourId, juce::Colours::grey);
    };

    static Font getComboBoxFont()
    {
        auto font = Font(Typeface::createSystemTypefaceFor(BinaryData::QuicksandBold_ttf, BinaryData::QuicksandBold_ttfSize));
        font.setSizeAndStyle(16, font.getAvailableStyles()[0], 1, 0);

        auto styles = font.getAvailableStyles();
        // DBG(styles[1]);
        return font;
    }

    static Font getPopupMenuFont()
    {
        auto font = Font(Typeface::createSystemTypefaceFor(BinaryData::QuicksandBold_ttf, BinaryData::QuicksandBold_ttfSize));
        font.setSizeAndStyle(32, font.getAvailableStyles()[0], 1, 0);
        return font;
    }

    // Path getTickShape(float height)
    // {
    //     static const unsigned char data[] =
    //         {
    //             109, 0, 224, 168, 68, 0, 0, 119, 67, 108, 0, 224, 172, 68, 0, 128, 146, 67, 113, 0, 192, 148, 68, 0, 0, 219, 67, 0, 96, 110, 68, 0, 224, 56, 68, 113, 0, 64, 51, 68, 0, 32, 130, 68, 0, 64, 20, 68, 0, 224,
    //             162, 68, 108, 0, 128, 3, 68, 0, 128, 168, 68, 113, 0, 128, 221, 67, 0, 192, 175, 68, 0, 0, 207, 67, 0, 32, 179, 68, 113, 0, 0, 201, 67, 0, 224, 173, 68, 0, 0, 181, 67, 0, 224, 161, 68, 108, 0, 128, 168, 67,
    //             0, 128, 154, 68, 113, 0, 128, 141, 67, 0, 192, 138, 68, 0, 128, 108, 67, 0, 64, 131, 68, 113, 0, 0, 62, 67, 0, 128, 119, 68, 0, 0, 5, 67, 0, 128, 114, 68, 113, 0, 0, 102, 67, 0, 192, 88, 68, 0, 128, 155,
    //             67, 0, 192, 88, 68, 113, 0, 0, 190, 67, 0, 192, 88, 68, 0, 128, 232, 67, 0, 224, 131, 68, 108, 0, 128, 246, 67, 0, 192, 139, 68, 113, 0, 64, 33, 68, 0, 128, 87, 68, 0, 0, 93, 68, 0, 224, 26, 68, 113, 0,
    //             96, 140, 68, 0, 128, 188, 67, 0, 224, 168, 68, 0, 0, 119, 67, 99, 101};

    //     return createPathFromData(height, data, sizeof(data));
    // }

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
};