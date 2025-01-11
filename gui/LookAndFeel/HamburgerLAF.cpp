#include "HamburgerLAF.h"

#include "juce_gui_basics/juce_gui_basics.h"
#include "BinaryData.h"

HamburgerLAF::HamburgerLAF(juce::Colour color) : knobColour(color)
{
    setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::black);
    setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::white);
    setColour(juce::Slider::thumbColourId, juce::Colours::whitesmoke);
    // setColour(Slider::rotarySliderFillColourId, color);

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



void HamburgerLAF::drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height, float sliderPos,
                        const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider &slider)
{
    auto thumb = slider.findColour(juce::Slider::thumbColourId);

    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(5.0f);

    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = juce::jmin(8.0f, radius * 0.5f);
    auto arcRadius = radius - lineW * 0.5f;

    juce::Line<float> marker;

    float xOffset = std::sin(toAngle) * arcRadius;
    float yOffset = -std::cos(toAngle) * arcRadius;

    marker.setStart(xOffset * 0.8f + bounds.getCentreX(), yOffset * 0.8f + bounds.getCentreY());
    marker.setEnd(xOffset + bounds.getCentreX(), yOffset + bounds.getCentreY());

    juce::Path p;
    p.addLineSegment(marker, radius * 0.08f);
    g.setColour(thumb);
    g.strokePath(p, juce::PathStrokeType(radius * 0.08f, juce::PathStrokeType::JointStyle::curved, juce::PathStrokeType::EndCapStyle::rounded));
}

void HamburgerLAF::drawComboBox(juce::Graphics &g, int width, int height, bool,
                    int, int, int, int, juce::ComboBox &box)
{
    auto cornerSize = box.findParentComponentOfClass<juce::ChoicePropertyComponent>() != nullptr ? 0.0f : 3.0f;
    juce::Rectangle<int> boxBounds(0, 0, width, height);

    // g.setColour(box.findColour(ComboBox::backgroundColourId));
    // g.fillRoundedRectangle(boxBounds.toFloat(), cornerSize);

    // g.setColour(box.findColour(ComboBox::outlineColourId));
    // g.drawRoundedRectangle(boxBounds.toFloat().reduced(0.5f, 0.5f), cornerSize, 1.0f);

    juce::Rectangle<int> arrowZone(5, 0, 20, height);
    juce::Path path;
    path.startNewSubPath((float)arrowZone.getX() + 4.0f, (float)arrowZone.getCentreY() - 2.0f);
    path.lineTo((float)arrowZone.getCentreX(), (float)arrowZone.getCentreY() + 3.0f);
    path.lineTo((float)arrowZone.getRight() - 4.0f, (float)arrowZone.getCentreY() - 2.0f);

    g.setColour(box.findColour(juce::ComboBox::arrowColourId).withAlpha((box.isEnabled() ? 0.9f : 0.2f)));
    g.strokePath(path, juce::PathStrokeType(2.0f, juce::PathStrokeType::JointStyle::curved, juce::PathStrokeType::EndCapStyle::rounded));
}

juce::Font HamburgerLAF::getQuicksandFont()
{
    return *quicksandFont;
}

juce::Font HamburgerLAF::getLabelFont(juce::Label &label)
{
    return *questrialFont14;
}

juce::Font HamburgerLAF::getComboBoxFont(juce::ComboBox &box)
{
    return *questrialFont14;
}

juce::Font HamburgerLAF::getPopupMenuFont()
{
    return *questrialFont16;
}

juce::Font HamburgerLAF::getComboBoxFont()
{

    return *questrialFont;
}

juce::Font HamburgerLAF::getAlertWindowFont()
{
    return *questrialFont;
}

juce::Font HamburgerLAF::getAlertWindowMessageFont()
{
    return *questrialFont;
}

juce::Font HamburgerLAF::getAlertWindowTitleFont()
{
    return *quicksandFont;
}

void HamburgerLAF::positionComboBoxText(juce::ComboBox &box, juce::Label &label)
{
    label.setBounds(30, 1,
                    box.getWidth() - 30,
                    box.getHeight() - 2);

    label.setFont(getComboBoxFont());
}

void HamburgerLAF::drawPopupMenuItem(juce::Graphics &g, const juce::Rectangle<int> &area,
                        const bool isSeparator, const bool isActive,
                        const bool isHighlighted, const bool isTicked,
                        const bool hasSubMenu, const juce::String &text,
                        const juce::String &shortcutKeyText,
                        const juce::Drawable *icon, const juce::Colour *const textColourToUse)
{
    if (isSeparator)
    {
        auto r = area.reduced(5, 0);
        r.removeFromTop(juce::roundToInt(((float)r.getHeight() * 0.5f) - 0.5f));

        g.setColour(findColour(juce::PopupMenu::textColourId).withAlpha(0.3f));
        g.fillRect(r.removeFromTop(1));
    }
    else
    {
        auto textColour = (textColourToUse == nullptr ? findColour(juce::PopupMenu::textColourId)
                                                        : *textColourToUse);

        auto r = area.reduced(1);

        if (isHighlighted && isActive)
        {
            g.setColour(findColour(juce::PopupMenu::highlightedBackgroundColourId));
            g.fillRect(r);

            g.setColour(findColour(juce::PopupMenu::highlightedTextColourId));
        }
        else
        {
            g.setColour(textColour.withMultipliedAlpha(isActive ? 1.0f : 0.5f));
        }

        r.reduce(juce::jmin(5, area.getWidth() / 20), 0);

        auto font = getPopupMenuFont();

        auto maxFontHeight = (float)r.getHeight() / 1.3f;

        if (font.getHeight() > maxFontHeight)
            font.setHeight(maxFontHeight);

        g.setFont(font);

        auto iconArea = r.removeFromLeft(juce::roundToInt(maxFontHeight)).toFloat();

        if (icon != nullptr)
        {
            icon->drawWithin(g, iconArea, juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize, 1.0f);
            r.removeFromLeft(juce::roundToInt(maxFontHeight * 0.5f));
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

            juce::Path path;
            path.startNewSubPath(x, halfH - arrowH * 0.5f);
            path.lineTo(x + arrowH * 0.6f, halfH);
            path.lineTo(x, halfH + arrowH * 0.5f);

            g.strokePath(path, juce::PathStrokeType(2.0f));
        }

        r.removeFromRight(3);
        g.drawFittedText(text, r, juce::Justification::centredLeft, 1);

        if (shortcutKeyText.isNotEmpty())
        {
            auto f2 = font;
            f2.setHeight(f2.getHeight() * 0.75f);
            f2.setHorizontalScale(0.95f);
            g.setFont(f2);

            g.drawText(shortcutKeyText, r, juce::Justification::centredRight, true);
        }
    }
}