#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

#include "BinaryData.h"

#include "../PluginProcessor.h"
#include "LookAndFeel/Theme.h"

class PluginHeader : public juce::Component,
                     public juce::TooltipClient
{
public:
    enum class Tab { start, advanced, pre, main, post, mod, presets, settings };

    static constexpr int pillInset = 4;
    static constexpr int totalHeight = 41;

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

    // what the editor keeps clear on the right, so the preset panel stops short of MOD and the gear
    int getRightReserved() const { return getWidth() - cells[firstRightCell()].getX(); }

    // START | ADVANCED while on the start page, START | PRE | MAIN | POST otherwise, with MOD | gear on the right
    void resized() override
    {
        tabs = simple ? std::vector<Tab> { Tab::start, Tab::advanced, Tab::mod, Tab::settings }
                      : std::vector<Tab> { Tab::start, Tab::pre, Tab::main, Tab::post, Tab::mod, Tab::settings };
        cells.resize (tabs.size());

        auto bar = tabRow();

        auto x = bar.getX() + edgePad;

        for (size_t i = 0; i < firstRightCell(); ++i)
        {
            const auto width = cellWidthFor (tabs[i]);

            cells[i] = { x, bar.getY(), width, bar.getHeight() };
            x += width;
        }

        auto right = bar.getRight();

        for (auto i = tabs.size(); i-- > firstRightCell();)
        {
            const auto width = cellWidthFor (tabs[i]);

            right -= width;
            cells[i] = { right, bar.getY(), width, bar.getHeight() };
        }
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (theme().box);
        g.fillRect (bar());

        auto bar = tabRow();

        g.setFont (font.withHeight (fontHeight));

        for (size_t i = 0; i < tabs.size(); ++i)
        {
            const auto cell = cells[i];
            const auto tab = tabs[i];

            g.setColour (tab == selectedTab ? theme().textHeader : ((int) i == hoveredCell ? theme().textHeaderHover : theme().textHeaderIdle));

            if (tab == Tab::settings)
                g.fillPath (settingsIcon, juce::RectanglePlacement (juce::RectanglePlacement::centred)
                                              .getTransformToFit ({ 0.0f, 0.0f, 24.0f, 24.0f }, cell.toFloat().withSizeKeepingCentre (iconSize, iconSize)));
            else if (showsMode (tab))
                drawMode (g, cell);
            else
                g.drawText (labelFor (tab), cell, juce::Justification::centred, false);

            // between the tabs on the left, and before each of the two on the right
            if (i >= firstRightCell())
                drawDivider (g, bar, (float) cell.getX());
            else if (i + 1 < firstRightCell())
                drawDivider (g, bar, (float) cell.getRight());
        }
    }

    void drawDivider (juce::Graphics& g, juce::Rectangle<int> bar, float x) const
    {
        g.setColour (theme().headerDivider);
        g.drawLine (x, (float) bar.getY() + dividerInset, x, (float) bar.getBottom() - dividerInset, 1.0f);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const auto i = cellAt (e.getPosition());

        if (i < 0)
            return;

        const auto tab = tabs[(size_t) i];

        if (showsMode (tab))
        {
            showModeMenu();
            return;
        }

        selectTab (tab);
    }

    // as if it were clicked, for opening on a page
    void selectTab (Tab tab)
    {
        // START swaps in the simpler header, ADVANCED brings back the full one and the main distortion with it
        if (tab == Tab::start || tab == Tab::advanced)
        {
            simple = tab == Tab::start;
            resized();
        }

        setSelectedTab (tab == Tab::advanced ? Tab::main : tab);

        if (onTabSelected != nullptr)
            onTabSelected (selectedTab);
    }

    void mouseMove (const juce::MouseEvent& e) override { setHovered (cellAt (e.getPosition())); }
    void mouseExit (const juce::MouseEvent&) override    { setHovered (-1); }

private:
    // flush with the top and sides of the window, with the gap under it that the boxes below would otherwise leave
    juce::Rectangle<int> bar() const { return getLocalBounds().withTrimmedBottom (pillInset); }

    juce::Rectangle<int> tabRow() const { return bar().reduced (pillInset, 0); }

    int cellWidthFor (Tab tab) const
    {
        if (tab == Tab::settings)
            return iconSize + cellPad * 2;

        const auto f = font.withHeight (fontHeight);
        auto width = juce::GlyphArrangement::getStringWidth (f, labelFor (tab));

        // widest it can ever hold, so selecting it or changing mode never reflows the bar
        if (tab == Tab::main)
            for (const auto& mode : modes)
                width = juce::jmax (width, juce::GlyphArrangement::getStringWidth (f, mode)
                                               + (float) chevronSpace);

        return juce::roundToInt (width) + cellPad * 2;
    }

    static juce::Path makeSettingsIcon()
    {
        static constexpr auto svg = R"svg(
            <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="#ffffff">
              <path stroke-linecap="round" stroke-linejoin="round" d="M9.594 3.94c.09-.542.56-.94 1.11-.94h2.593c.55 0 1.02.398 1.11.94l.213 1.281c.063.374.313.686.645.87.074.04.147.083.22.127.325.196.72.257 1.075.124l1.217-.456a1.125 1.125 0 0 1 1.37.49l1.296 2.247a1.125 1.125 0 0 1-.26 1.431l-1.003.827c-.293.241-.438.613-.43.992a7.723 7.723 0 0 1 0 .255c-.008.378.137.75.43.991l1.004.827c.424.35.534.955.26 1.43l-1.298 2.247a1.125 1.125 0 0 1-1.369.491l-1.217-.456c-.355-.133-.75-.072-1.076.124a6.47 6.47 0 0 1-.22.128c-.331.183-.581.495-.644.869l-.213 1.281c-.09.543-.56.94-1.11.94h-2.594c-.55 0-1.019-.398-1.11-.94l-.213-1.281c-.062-.374-.312-.686-.644-.87a6.52 6.52 0 0 1-.22-.127c-.325-.196-.72-.257-1.076-.124l-1.217.456a1.125 1.125 0 0 1-1.369-.49l-1.297-2.247a1.125 1.125 0 0 1 .26-1.431l1.004-.827c.292-.24.437-.613.43-.991a6.932 6.932 0 0 1 0-.255c.007-.38-.138-.751-.43-.992l-1.004-.827a1.125 1.125 0 0 1-.26-1.43l1.297-2.247a1.125 1.125 0 0 1 1.37-.491l1.216.456c.356.133.751.072 1.076-.124.072-.044.146-.086.22-.128.332-.183.582-.495.644-.869l.214-1.28Z" />
              <path stroke-linecap="round" stroke-linejoin="round" d="M15 12a3 3 0 1 1-6 0 3 3 0 0 1 6 0Z" />
            </svg>
        )svg";

        auto parsed = juce::XmlDocument::parse (juce::String (svg));
        jassert (parsed != nullptr);

        return juce::Drawable::createFromSVG (*parsed)->getOutlineAsPath();
    }

    // MOD and the gear, the last two, sit on the right with the preset panel between them and the rest
    size_t firstRightCell() const noexcept { return tabs.size() - 2; }

    bool showsMode (Tab tab) const noexcept { return tab == Tab::main && selectedTab == Tab::main; }

    static juce::String labelFor (Tab tab)
    {
        switch (tab)
        {
            case Tab::start:    return "START";
            case Tab::advanced: return "ADVANCED";
            case Tab::pre:      return "PRE";
            case Tab::main:     return "MAIN";
            case Tab::post:     return "POST";
            case Tab::mod:      return "MOD";
            case Tab::presets:
            case Tab::settings: break;
        }

        return {};
    }

    // the parameter's own list: the index picked here is the index stored, so it can't be a copy
    const juce::StringArray& modes = ParamIDs::routingTypes.categories;

    int currentMode() const { return routing != nullptr ? routing->getIndex() : 0; }

    juce::AudioParameterChoice* routing = nullptr;
    std::unique_ptr<juce::ParameterAttachment> routingAttachment;

    const juce::Path settingsIcon = makeSettingsIcon();

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

        const auto mainCell = (size_t) std::distance (tabs.begin(), std::find (tabs.begin(), tabs.end(), Tab::main));

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

        const auto tab = tabs[(size_t) i];

        switch (tab)
        {
            case Tab::start:
                return "A simpler view with just the essentials.";
            case Tab::advanced:
                return "Every control Hamburger has.";
            case Tab::pre:
                return "Distortion before the main one.";
            case Tab::main:
                return showsMode (tab)
                    ? "The main distortion: up to four at once. Click to change how they're arranged."
                    : "The main distortion";
            case Tab::post:
                return "Distortion after the main one.";
            case Tab::mod:
                return "Where modulation gets routed to the controls.";
            case Tab::settings:
                return "Click to open the Settings page";
            case Tab::presets:
                break;
        }

        return {};
    }

    int cellAt (juce::Point<int> p) const
    {
        for (size_t i = 0; i < cells.size(); ++i)
            if (cells[i].contains (p))
                return (int) i;

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
    bool simple = false;
    int hoveredCell = -1;

    std::vector<Tab> tabs;
    std::vector<juce::Rectangle<int>> cells;

    const juce::Typeface::Ptr typeface = juce::Typeface::createSystemTypefaceFor (
        BinaryData::QuestrialRegular_ttf, BinaryData::QuestrialRegular_ttfSize);
    const juce::Font font { juce::FontOptions (typeface) };

    static constexpr float fontHeight = 15.0f;
    static constexpr int edgePad = 12;
    static constexpr int cellPad = 16;
    static constexpr int chevronSpace = 16;
    static constexpr float dividerInset = 12.0f;

    static constexpr int iconSize = 14;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginHeader)
};
