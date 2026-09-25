#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

#include "BinaryData.h"

#include "../PluginProcessor.h"

class PluginHeader : public juce::Component,
                     public juce::TooltipClient
{
public:
    enum class Tab { pre, main, post, presets, settings };

    static constexpr int pillInset = 4;
    static constexpr int totalHeight = 45;

    // what the editor has to keep clear on the right so the gear isn't sat on
    static constexpr int rightReserved = 14 + 16 * 2 + 12;

    std::function<void (Tab)> onTabSelected;

    explicit PluginHeader (AudioPluginAudioProcessor& p)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);

        routing = dynamic_cast<juce::AudioParameterChoice*> (
            p.treeState.getParameter (ParamIDs::mainRouting.getParamID()));
        jassert (routing);

        /*  Attached rather than just written to, so automation and preset loads move the label
            too instead of it only tracking what was picked from this menu. */
        if (routing != nullptr)
            routingAttachment = std::make_unique<juce::ParameterAttachment> (
                *routing, [this] (float) { repaint(); }, nullptr);
    }

    void setSelectedTab (Tab tab)
    {
        selectedTab = tab;
        repaint();
    }

    Tab getSelectedTab() const noexcept { return selectedTab; }

    void resized() override
    {
        auto bar = tabRow();

        auto x = bar.getX() + edgePad;

        for (int i = 0; i < settingsCell; ++i)
        {
            const auto width = cellWidthFor (i);

            cells[(size_t) i] = { x, bar.getY(), width, bar.getHeight() };
            x += width;
        }

        const auto gear = cellWidthFor (settingsCell);
        cells[(size_t) settingsCell] = { bar.getRight() - gear, bar.getY(), gear, bar.getHeight() };
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle (pill().toFloat(), 15.0f);

        auto bar = tabRow();

        g.setFont (font.withHeight (fontHeight));

        for (int i = 0; i < numCells; ++i)
        {
            const auto cell = cells[(size_t) i];
            const auto active = tabFor (i) == selectedTab;
            const auto alpha = active ? 1.0f : (i == hoveredCell ? hoverAlpha : inactiveAlpha);

            g.setColour (juce::Colours::white.withAlpha (alpha));

            if (isSettingsCell (i))
                settingsIcon->drawWithin (g, cell.toFloat().withSizeKeepingCentre (iconSize, iconSize),
                                          juce::RectanglePlacement::centred, alpha);
            else if (showsMode (i))
                drawMode (g, cell);
            else
                g.drawText (labels[(size_t) i], cell, juce::Justification::centred, false);

            if (i < postCell)
                drawDivider (g, bar, (float) cell.getRight());
        }

        drawDivider (g, bar, (float) cells[(size_t) settingsCell].getX());
    }

    void drawDivider (juce::Graphics& g, juce::Rectangle<int> bar, float x) const
    {
        g.setColour (dividerColour);
        g.drawLine (x, (float) bar.getY() + dividerInset, x, (float) bar.getBottom() - dividerInset, 1.0f);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const auto i = cellAt (e.getPosition());

        if (i < 0)
            return;

        if (showsMode (i))
        {
            showModeMenu();
            return;
        }

        setSelectedTab (tabFor (i));

        if (onTabSelected != nullptr)
            onTabSelected (selectedTab);
    }

    void mouseMove (const juce::MouseEvent& e) override { setHovered (cellAt (e.getPosition())); }
    void mouseExit (const juce::MouseEvent&) override    { setHovered (-1); }

private:
    juce::Rectangle<int> pill() const { return getLocalBounds().reduced (pillInset); }

    juce::Rectangle<int> tabRow() const { return pill(); }

    int cellWidthFor (int i) const
    {
        if (isSettingsCell (i))
            return iconSize + cellPad * 2;

        const auto f = font.withHeight (fontHeight);
        auto width = juce::GlyphArrangement::getStringWidth (f, labels[i]);

        // widest it can ever hold, so selecting it or changing mode never reflows the bar
        if (i == mainCell)
            for (const auto& mode : modes)
                width = juce::jmax (width, juce::GlyphArrangement::getStringWidth (f, mode)
                                               + (float) chevronSpace);

        return juce::roundToInt (width) + cellPad * 2;
    }

    static std::unique_ptr<juce::Drawable> makeSettingsIcon()
    {
        static constexpr auto svg = R"svg(
            <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="#ffffff">
              <path stroke-linecap="round" stroke-linejoin="round" d="M9.594 3.94c.09-.542.56-.94 1.11-.94h2.593c.55 0 1.02.398 1.11.94l.213 1.281c.063.374.313.686.645.87.074.04.147.083.22.127.325.196.72.257 1.075.124l1.217-.456a1.125 1.125 0 0 1 1.37.49l1.296 2.247a1.125 1.125 0 0 1-.26 1.431l-1.003.827c-.293.241-.438.613-.43.992a7.723 7.723 0 0 1 0 .255c-.008.378.137.75.43.991l1.004.827c.424.35.534.955.26 1.43l-1.298 2.247a1.125 1.125 0 0 1-1.369.491l-1.217-.456c-.355-.133-.75-.072-1.076.124a6.47 6.47 0 0 1-.22.128c-.331.183-.581.495-.644.869l-.213 1.281c-.09.543-.56.94-1.11.94h-2.594c-.55 0-1.019-.398-1.11-.94l-.213-1.281c-.062-.374-.312-.686-.644-.87a6.52 6.52 0 0 1-.22-.127c-.325-.196-.72-.257-1.076-.124l-1.217.456a1.125 1.125 0 0 1-1.369-.49l-1.297-2.247a1.125 1.125 0 0 1 .26-1.431l1.004-.827c.292-.24.437-.613.43-.991a6.932 6.932 0 0 1 0-.255c.007-.38-.138-.751-.43-.992l-1.004-.827a1.125 1.125 0 0 1-.26-1.43l1.297-2.247a1.125 1.125 0 0 1 1.37-.491l1.216.456c.356.133.751.072 1.076-.124.072-.044.146-.086.22-.128.332-.183.582-.495.644-.869l.214-1.28Z" />
              <path stroke-linecap="round" stroke-linejoin="round" d="M15 12a3 3 0 1 1-6 0 3 3 0 0 1 6 0Z" />
            </svg>
        )svg";

        auto parsed = juce::XmlDocument::parse (juce::String (svg));
        jassert (parsed != nullptr);

        return juce::Drawable::createFromSVG (*parsed);
    }

    // PRE | MAIN | POST packed left, the gear alone on the right
    static constexpr int numCells = 4;
    static constexpr int mainCell = 1;
    static constexpr int postCell = 2;
    static constexpr int settingsCell = 3;

    bool showsMode (int i) const noexcept       { return i == mainCell && selectedTab == Tab::main; }
    static bool isSettingsCell (int i) noexcept { return i == settingsCell; }

    static Tab tabFor (int i) noexcept
    {
        switch (i)
        {
            case 0:  return Tab::pre;
            case 1:  return Tab::main;
            case 2:  return Tab::post;
            default: return Tab::settings;
        }
    }

    // the parameter's own list: the index picked here is the index stored, so it can't be a copy
    const juce::StringArray& modes = ParamIDs::routingTypes.categories;

    juce::StringArray labels { "PRE", "MAIN", "POST", "" }; // settings is an icon

    int currentMode() const { return routing != nullptr ? routing->getIndex() : 0; }

    juce::AudioParameterChoice* routing = nullptr;
    std::unique_ptr<juce::ParameterAttachment> routingAttachment;

    std::unique_ptr<juce::Drawable> settingsIcon = makeSettingsIcon();

    void drawMode (juce::Graphics& g, juce::Rectangle<int> cell) const
    {
        const auto text = modes[currentMode()];
        const auto textWidth = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), text);

        // shifted right by half the chevron so the pair centres, not the text alone
        const auto textArea = cell.toFloat().translated ((float) chevronSpace * 0.5f, 0.0f);
        const auto centre = textArea.getCentre();
        const auto left = centre.x - textWidth * 0.5f;

        g.drawText (text, textArea, juce::Justification::centred, false);

        const auto underlineY = centre.y + 9.0f;
        g.drawLine (left, underlineY, left + textWidth, underlineY, 1.0f);

        juce::Path chevron;
        const auto cx = left - 12.0f;
        chevron.startNewSubPath (cx - 4.0f, centre.y - 2.0f);
        chevron.lineTo (cx, centre.y + 2.0f);
        chevron.lineTo (cx + 4.0f, centre.y - 2.0f);

        g.strokePath (chevron, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
    }

    void showModeMenu()
    {
        juce::Component::SafePointer<PluginHeader> safeThis (this);

        juce::PopupMenu menu;
        menu.setLookAndFeel (&getLookAndFeel());

        for (int i = 0; i < modes.size(); ++i)
            menu.addItem (modes[i], true, i == currentMode(), [safeThis, i]
            {
                if (safeThis != nullptr && safeThis->routingAttachment != nullptr)
                    safeThis->routingAttachment->setValueAsCompleteGesture ((float) i);
            });

        menu.showMenuAsync (juce::PopupMenu::Options()
                                .withTargetComponent (this)
                                .withTargetScreenArea (localAreaToGlobal (cells[mainCell])));
    }

    // one component, so the tip is for whichever tab is under the mouse
    juce::String getTooltip() override
    {
        const auto i = cellAt (getMouseXYRelative());

        if (i < 0)
            return {};

        switch (tabFor (i))
        {
            case Tab::pre:
                return "Distortion before the main one.";
            case Tab::main:
                return showsMode (i)
                    ? "The main distortion: up to four at once. Click to change how they're arranged."
                    : "The main distortion";
            case Tab::post:
                return "Distortion after the main one.";
            case Tab::settings:
                return "Click to open the Settings page";
            case Tab::presets:
                break;
        }

        return {};
    }

    int cellAt (juce::Point<int> p) const
    {
        for (int i = 0; i < numCells; ++i)
            if (cells[(size_t) i].contains (p))
                return i;

        return -1;
    }

    void setHovered (int i)
    {
        if (i == hoveredCell)
            return;

        hoveredCell = i;
        repaint();
    }

    Tab selectedTab = Tab::main;
    int hoveredCell = -1;

    std::array<juce::Rectangle<int>, numCells> cells;

    const juce::Typeface::Ptr typeface = juce::Typeface::createSystemTypefaceFor (
        BinaryData::QuestrialRegular_ttf, BinaryData::QuestrialRegular_ttfSize);
    const juce::Font font { juce::FontOptions (typeface) };

    static constexpr float fontHeight = 15.0f;
    static constexpr int edgePad = 12;
    static constexpr int cellPad = 16;
    static constexpr int chevronSpace = 16;
    static constexpr float dividerInset = 12.0f;

    static constexpr int iconSize = 14;
    // white at these alphas over black matches the text greys, so the icon shares them
    static constexpr float inactiveAlpha = 0.43f;
    static constexpr float hoverAlpha = 0.67f;

    const juce::Colour dividerColour { juce::Colour::fromRGB (74, 74, 74) };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginHeader)
};
