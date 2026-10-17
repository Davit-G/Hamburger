#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

#include "BinaryData.h"

class HeaderTabs : public juce::Component,
                   public juce::TooltipClient
{
public:
    enum Tab { pre, main, post, settings };

    static constexpr int pillInset = 4;
    static constexpr int height = 41;

    std::function<void (Tab)> onTabSelected;

    // the colour of the distortion type showing on PRE or POST
    std::function<juce::Colour (Tab)> typeColour;

    HeaderTabs() { setMouseCursor (juce::MouseCursor::PointingHandCursor); }

    void select (Tab tab)
    {
        selected = tab;
        repaint();

        if (onTabSelected != nullptr)
            onTabSelected (tab);
    }

    // what the preset bar keeps clear either side of it
    int getLeftReserved() const { return cells[post].getRight(); }
    int getRightReserved() const { return getWidth() - cells[settings].getX(); }

    void resized() override
    {
        auto row = getLocalBounds().withTrimmedBottom (pillInset).reduced (pillInset, 0);
        cells[settings] = row.removeFromRight (iconSize + cellPad * 2);

        row.removeFromLeft (edgePad);

        for (auto tab : { pre, main, post })
            cells[tab] = row.removeFromLeft (juce::roundToInt (juce::GlyphArrangement::getStringWidth (font, labels[tab])) + cellPad * 2);
    }

    void paint (juce::Graphics& g) override
    {
        // flush with the top and sides of the window, with the gap under it that the boxes below would otherwise leave
        const auto bar = getLocalBounds().withTrimmedBottom (pillInset);

        g.setColour(juce::Colours::black);
        g.fillRect(bar);

        g.setFont(font);

        for (auto tab : { pre, main, post, settings })
        {
            const auto cell = cells[tab];

            if (tab == selected)
            {
                g.setColour(tab == main ? juce::Colours::white : tab != settings && typeColour != nullptr ? typeColour(tab) : juce::Colour (0xffffa746));
                g.fillRoundedRectangle(cell.toFloat().reduced (10.0f, 0.0f).removeFromBottom(5.0f).removeFromTop(2.0f), 1.0f);
            }

            g.setColour(juce::Colours::white.withAlpha (tab == selected ? 1.0f : tab == hovered ? 0.67f : 0.43f));

            if (tab == settings) {
                g.fillPath(gear, juce::RectanglePlacement(juce::RectanglePlacement::centred).getTransformToFit({ 0.0f, 0.0f, 24.0f, 24.0f }, cell.toFloat().withSizeKeepingCentre(iconSize, iconSize)));
            } else {
                g.drawText(labels[tab], cell, juce::Justification::centred, false);
            }
        }

        // between the tabs on the left, and before the gear
        g.setColour(juce::Colour (0xff4a4a4a));

        for (auto x : { cells[pre].getRight(), cells[main].getRight(), cells[settings].getX() })
            g.drawLine((float) x, (float) bar.getY() + dividerInset, (float) x, (float) bar.getBottom() - dividerInset, 1.0f);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (auto tab = tabAt (e.getPosition()); tab >= 0)
            select ((Tab) tab);
    }

    void mouseMove (const juce::MouseEvent& e) override { setHovered (tabAt (e.getPosition())); }
    void mouseExit (const juce::MouseEvent&) override { setHovered (-1); }

    juce::String getTooltip() override
    {
        switch (tabAt (getMouseXYRelative()))
        {
            case pre: return "Distortion before the main one.";
            case main: return "The main distortion";
            case post: return "Distortion after the main one.";
            case settings: return "Click to open the Settings page";
            default: return {};
        }
    }

private:
    int tabAt (juce::Point<int> p) const
    {
        for (int tab = 0; tab < (int) cells.size(); ++tab)
            if (cells[(size_t) tab].contains (p))
                return tab;

        return -1;
    }

    void setHovered (int tab)
    {
        if (tab == hovered)
            return;

        hovered = tab;
        repaint();
    }

    Tab selected = main;
    int hovered = -1;

    std::array<juce::Rectangle<int>, 4> cells;

    static constexpr const char* labels[] { "PRE", "MAIN", "POST" };

    const juce::Font font = juce::Font (juce::FontOptions (juce::Typeface::createSystemTypefaceFor (
        BinaryData::QuestrialRegular_ttf, BinaryData::QuestrialRegular_ttfSize))).withHeight (15.0f);

    static constexpr int edgePad = 12;
    static constexpr int cellPad = 16;
    static constexpr float dividerInset = 12.0f;
    static constexpr int iconSize = 14;

    const juce::Path gear = []
    {
        static constexpr auto svg = R"svg(
            <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="#ffffff">
              <path stroke-linecap="round" stroke-linejoin="round" d="M9.594 3.94c.09-.542.56-.94 1.11-.94h2.593c.55 0 1.02.398 1.11.94l.213 1.281c.063.374.313.686.645.87.074.04.147.083.22.127.325.196.72.257 1.075.124l1.217-.456a1.125 1.125 0 0 1 1.37.49l1.296 2.247a1.125 1.125 0 0 1-.26 1.431l-1.003.827c-.293.241-.438.613-.43.992a7.723 7.723 0 0 1 0 .255c-.008.378.137.75.43.991l1.004.827c.424.35.534.955.26 1.43l-1.298 2.247a1.125 1.125 0 0 1-1.369.491l-1.217-.456c-.355-.133-.75-.072-1.076.124a6.47 6.47 0 0 1-.22.128c-.331.183-.581.495-.644.869l-.213 1.281c-.09.543-.56.94-1.11.94h-2.594c-.55 0-1.019-.398-1.11-.94l-.213-1.281c-.062-.374-.312-.686-.644-.87a6.52 6.52 0 0 1-.22-.127c-.325-.196-.72-.257-1.076-.124l-1.217.456a1.125 1.125 0 0 1-1.369-.49l-1.297-2.247a1.125 1.125 0 0 1 .26-1.431l1.004-.827c.292-.24.437-.613.43-.991a6.932 6.932 0 0 1 0-.255c.007-.38-.138-.751-.43-.992l-1.004-.827a1.125 1.125 0 0 1-.26-1.43l1.297-2.247a1.125 1.125 0 0 1 1.37-.491l1.216.456c.356.133.751.072 1.076-.124.072-.044.146-.086.22-.128.332-.183.582-.495.644-.869l.214-1.28Z" />
              <path stroke-linecap="round" stroke-linejoin="round" d="M15 12a3 3 0 1 1-6 0 3 3 0 0 1 6 0Z" />
            </svg>
        )svg";

        return juce::Drawable::createFromSVG (*juce::XmlDocument::parse (svg))->getOutlineAsPath();
    }();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeaderTabs)
};
