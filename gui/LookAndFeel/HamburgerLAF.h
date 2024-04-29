#pragma once


class HamburgerLAF : public juce::LookAndFeel_V4
{
public:
    HamburgerLAF(juce::Colour color = juce::Colour::fromRGB(50, 255, 205)) : knobColour(color)
    {
        setColour(Slider::rotarySliderOutlineColourId, juce::Colours::black);
        setColour(Slider::rotarySliderFillColourId, juce::Colours::white);
        setColour(Slider::thumbColourId, juce::Colours::whitesmoke);
        setColour(Slider::rotarySliderFillColourId, color);

        setColour(juce::ComboBox::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::ComboBox::ColourIds::buttonColourId, juce::Colours::transparentBlack);
        setColour(juce::ComboBox::ColourIds::outlineColourId, juce::Colours::transparentBlack);
        setColour(juce::ComboBox::ColourIds::textColourId, juce::Colours::white);
        setColour(juce::ComboBox::arrowColourId, juce::Colours::white);

        // setColour(juce::ComboBox::ColourIds::focusedOutlineColourId, juce::Colours::transparentBlack);

        setColour(juce::PopupMenu::ColourIds::backgroundColourId, juce::Colours::black);
        setColour(juce::PopupMenu::ColourIds::textColourId, juce::Colours::white);
        setColour(juce::PopupMenu::ColourIds::highlightedBackgroundColourId, juce::Colour::fromHSV(0.0f, 0.0f, 0.2f, 1.0f));
        setColour(juce::PopupMenu::ColourIds::highlightedTextColourId, juce::Colours::white);

        // dont use setsizeandstyle, it will remove the typeface info for windows specifically (weird bug)

        #if JUCE_WINDOWS
            questrialFont->setHeight(20.0f);
            questrialFont12->setHeight(18.0f);
            questrialFont14->setHeight(20.0f);
            questrialFont16->setHeight(22.0f);

            quicksandFont->setHeight(22.0f);
        #else
            questrialFont->setHeight(14.0f);
            questrialFont12->setHeight(12.0f);
            questrialFont14->setHeight(14.0f);
            questrialFont16->setHeight(16.0f);

            quicksandFont->setHeight(16.0f);
        #endif
    }


    const juce::Typeface::Ptr questrialTypeface = juce::Typeface::createSystemTypefaceFor(BinaryData::QuestrialRegular_ttf, BinaryData::QuestrialRegular_ttfSize);
    const juce::Typeface::Ptr quicksandTypeface = juce::Typeface::createSystemTypefaceFor(BinaryData::QuicksandBold_ttf, BinaryData::QuicksandBold_ttfSize);

    const std::unique_ptr<Font> questrialFont = std::make_unique<juce::Font>(questrialTypeface);
    const std::unique_ptr<Font> questrialFont12 = std::make_unique<juce::Font>(questrialTypeface);
    const std::unique_ptr<Font> questrialFont14 = std::make_unique<juce::Font>(questrialTypeface);
    const std::unique_ptr<Font> questrialFont16 = std::make_unique<juce::Font>(questrialTypeface);
    const std::unique_ptr<Font> quicksandFont = std::make_unique<juce::Font>(quicksandTypeface);

    Font getQuicksandFont()
    {
        return *quicksandFont;
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
        auto thumb = slider.findColour(Slider::thumbColourId);

        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(5.0f);

        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        auto lineW = juce::jmin(8.0f, radius * 0.5f);
        auto arcRadius = radius - lineW * 0.5f;

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

    void drawComboBox(Graphics &g, int width, int height, bool,
                      int, int, int, int, ComboBox &box) override
    {
        auto cornerSize = box.findParentComponentOfClass<ChoicePropertyComponent>() != nullptr ? 0.0f : 3.0f;
        juce::Rectangle<int> boxBounds(0, 0, width, height);

        // g.setColour(box.findColour(ComboBox::backgroundColourId));
        // g.fillRoundedRectangle(boxBounds.toFloat(), cornerSize);

        // g.setColour(box.findColour(ComboBox::outlineColourId));
        // g.drawRoundedRectangle(boxBounds.toFloat().reduced(0.5f, 0.5f), cornerSize, 1.0f);

        juce::Rectangle<int> arrowZone(5, 0, 20, height);
        Path path;
        path.startNewSubPath((float)arrowZone.getX() + 4.0f, (float)arrowZone.getCentreY() - 2.0f);
        path.lineTo((float)arrowZone.getCentreX(), (float)arrowZone.getCentreY() + 3.0f);
        path.lineTo((float)arrowZone.getRight() - 4.0f, (float)arrowZone.getCentreY() - 2.0f);

        g.setColour(box.findColour(ComboBox::arrowColourId).withAlpha((box.isEnabled() ? 0.9f : 0.2f)));
        g.strokePath(path, PathStrokeType(2.0f, juce::PathStrokeType::JointStyle::curved, juce::PathStrokeType::EndCapStyle::rounded));
    }

    Font getComboBoxFont()
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

    void positionComboBoxText(ComboBox &box, Label &label) override
    {
        label.setBounds(30, 1,
                        box.getWidth() - 30,
                        box.getHeight() - 2);

        label.setFont(getComboBoxFont());
    }

    void drawPopupMenuItem(Graphics &g, const juce::Rectangle<int> &area,
                           const bool isSeparator, const bool isActive,
                           const bool isHighlighted, const bool isTicked,
                           const bool hasSubMenu, const String &text,
                           const String &shortcutKeyText,
                           const Drawable *icon, const Colour *const textColourToUse) override
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
    juce::Colour knobColour;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HamburgerLAF);
};