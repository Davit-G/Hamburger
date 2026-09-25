#pragma once

#include "../PluginProcessor.h"
#include "LookAndFeel/HamburgerLAF.h"
#include "LookAndFeel/ThemeManager.h"
#include "Modules/Panel.h"
#include "ThemeCustomiser.h"

class FxOrderList : public juce::Component,
                    private juce::Timer
{
public:
    explicit FxOrderList (AudioPluginAudioProcessor& p) : processor (p)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);

        startTimerHz (10);
    }

    // the colour a module's name is written in, its box's current colour
    std::function<juce::Colour (ModuleId)> colourFor;

    void paint (juce::Graphics& g) override
    {
        const auto order = shownOrder();

        auto font = juce::Font (juce::FontOptions {}).boldened();

        if (auto* laf = dynamic_cast<HamburgerLAF*> (&getLookAndFeel()))
            font = laf->getQuicksandFont();

        for (int row = 0; row < numRows; ++row)
        {
            const auto area = rowArea (row);
            const auto id = order[(size_t) row];

            // the dragged row lit up, so it's clear which one is in hand
            g.setColour (row == dragRow ? theme().rowDragged : theme().row);
            g.fillRoundedRectangle (area.reduced (0.0f, 1.5f), 4.0f);

            auto text = area.reduced (8.0f, 0.0f);

            g.setFont (font.withHeight (juce::jmin (15.0f, area.getHeight() * 0.6f)));
            g.setColour (theme().textSettingsDim);
            g.drawText (juce::String (row + 1), text.removeFromLeft (20.0f), juce::Justification::centredLeft, false);

            g.setColour (colourFor != nullptr ? colourFor (id) : theme().textSettings);
            g.drawText (nameFor (id), text, juce::Justification::centredLeft, false);
        }
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        setMouseCursor (isPinned (processor.getRoutingOrder()[(size_t) rowAt (e.position.y)])
                            ? juce::MouseCursor::NormalCursor : juce::MouseCursor::PointingHandCursor);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        committed = processor.getRoutingOrder();
        preview = committed;

        const auto row = rowAt (e.position.y);

        if (isPinned (committed[(size_t) row]))
            return;

        dragRow = row;

        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        repaint();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragRow < 0)
            return;

        auto target = rowAt (e.position.y);

        // a move only shifts the rows between where it was and where it's going, so stopping short of the pinned
        // row is all it takes to leave it in place
        const auto pinned = pinnedRow (preview);

        if (pinned >= 0)
            target = dragRow < pinned ? juce::jmin (target, pinned - 1) : juce::jmax (target, pinned + 1);

        if (target == dragRow)
            return;

        // the rows between slide one place towards where it came from
        const auto begin = preview.begin();

        if (target > dragRow)
            std::rotate (begin + dragRow, begin + dragRow + 1, begin + target + 1);
        else
            std::rotate (begin + target, begin + dragRow, begin + dragRow + 1);

        dragRow = target;
        repaint();
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (dragRow >= 0 && preview != committed)
            processor.setRoutingOrder (preview);

        dragRow = -1;
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        repaint();
    }

private:
    static constexpr int numRows = (int) ModuleId::count;
    static constexpr float maxRowHeight = 30.0f;

    static bool isPinned (ModuleId id) { return id == ModuleId::postClip; }

    static int pinnedRow (const RoutingOrder& order)
    {
        for (int row = 0; row < numRows; ++row)
            if (isPinned (order[(size_t) row]))
                return row;

        return -1;
    }

    RoutingOrder shownOrder() const { return dragRow >= 0 ? preview : processor.getRoutingOrder(); }

    float rowHeight() const { return juce::jmin (maxRowHeight, (float) getHeight() / (float) numRows); }

    juce::Rectangle<float> rowArea (int row) const
    {
        return { 0.0f, (float) row * rowHeight(), (float) getWidth(), rowHeight() };
    }

    int rowAt (float y) const { return juce::jlimit (0, numRows - 1, (int) std::floor (y / rowHeight())); }

    // what each module is running right now: main by its routing, the rest by their type where they have one
    juce::String nameFor (ModuleId id) const
    {
        switch (id)
        {
            case ModuleId::preEmphasis:    return "PRE-EMPHASIS";
            // as their boxes' type menus read
            case ModuleId::dynamics:       return typeName (id) + " COMP";
            case ModuleId::module1:        return typeName (id) + " NOISE";
            case ModuleId::module2:        return typeName (id);
            case ModuleId::preDistortion:  return "PRE " + typeName (id);
            case ModuleId::main:
            {
                auto* routing = choice (ParamIDs::mainRouting.getParamID());
                return routing != nullptr ? routing->getCurrentChoiceName() : "MAIN";
            }
            case ModuleId::postDistortion: return "POST " + typeName (id);
            case ModuleId::postEmphasis:   return "POST-EMPHASIS";
            case ModuleId::postClip:       return "CLIP";
            case ModuleId::count:          break;
        }

        return {};
    }

    juce::AudioParameterChoice* choice (const juce::String& paramId) const
    {
        return dynamic_cast<juce::AudioParameterChoice*> (processor.treeState.getParameter (paramId));
    }

    // the type the module's box is showing
    juce::String typeName (ModuleId id) const
    {
        auto* type = choice (SlotId { id, 0 }.type().getParamID());
        return type != nullptr ? type->getCurrentChoiceName() : juce::String();
    }

    juce::StringArray currentNames() const
    {
        juce::StringArray names;

        for (int i = 0; i < numRows; ++i)
            names.add (nameFor ((ModuleId) i));

        return names;
    }

    // a preset, a project load or a module's own type or routing can change what's listed
    void timerCallback() override
    {
        const auto current = processor.getRoutingOrder();
        const auto names = currentNames();

        if (dragRow < 0 && (current != lastPainted || names != lastNames))
        {
            lastPainted = current;
            lastNames = names;
            repaint();
        }
    }

    AudioPluginAudioProcessor& processor;

    RoutingOrder committed {}, preview {}, lastPainted {};
    juce::StringArray lastNames;
    int dragRow = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxOrderList)
};

class SettingsPanel : public juce::Component,
                      private juce::ValueTree::Listener,
                      private juce::AsyncUpdater,
                      private juce::ChangeListener
{
public:
    explicit SettingsPanel (AudioPluginAudioProcessor& p) : processorRef (p), fxOrder (p)
    {
        for (auto* label : allLabels())
        {
            label->setJustificationType (juce::Justification::centredLeft);
            addAndMakeVisible (label);
        }

        title.setText ("SETTINGS", juce::dontSendNotification);
        HamburgerLAF::setLabelFontScale (title, 1.4f);

        tooltipTitle.setText ("TOOLTIPS", juce::dontSendNotification);
        presetFolderTitle.setText ("PRESET FOLDER", juce::dontSendNotification);
        crossoverTitle.setText ("CROSSOVER SLOPE", juce::dontSendNotification);
        fxOrderTitle.setText ("FX ORDER", juce::dontSendNotification);
        themeTitle.setText ("THEME", juce::dontSendNotification);

        tooltipType.addItem ("None", 1);
        tooltipType.addItem ("Hovering", 2);
        tooltipType.addItem ("Replace Logo", 3);
        tooltipType.setSelectedId (static_cast<int> (processorRef.getAppProperties().getTooltipType()) + 1, juce::dontSendNotification);
        tooltipType.onChange = [this] {
            processorRef.getAppProperties().setTooltipType (static_cast<AppProperties::TooltipType> (tooltipType.getSelectedId() - 1));
        };
        addAndMakeVisible (tooltipType);

        changePresetFolder.onClick = [this] { choosePresetFolder(); };
        addAndMakeVisible (changePresetFolder);

        // in the plugin state rather than the app's settings, so it saves with the preset or project like the sound does
        for (const auto slope : { 12, 24, 48 })
            crossoverSlope.addItem (juce::String (slope) + " dB/oct", slope);

        crossoverSlope.setTooltip ("How steep the multiband and exciter band splits are. Saved with the preset");
        crossoverSlope.onChange = [this] {
            processorRef.treeState.state.setProperty (MainRouting::crossoverSlopeProperty, crossoverSlope.getSelectedId(), nullptr);
        };
        addAndMakeVisible (crossoverSlope);

        // the state tree gets swapped whole on a preset or project load, which is valueTreeRedirected
        processorRef.treeState.state.addListener (this);
        showCrossoverSlope();

        addAndMakeVisible (fxOrder);

        themeSelector.onChange = [this] {
            if (const auto index = themeSelector.getSelectedId() - 1; juce::isPositiveAndBelow (index, themeEntries.size()))
                themes->select (themeEntries[index].id);
        };
        addAndMakeVisible (themeSelector);

        customiseTheme.setTooltip ("Opens a window with every colour in the theme");
        customiseTheme.onClick = [this] {
            if (customiser == nullptr)
                customiser = std::make_unique<ThemeCustomiser>();

            customiser->setVisible (true);
            customiser->toFront (true);
        };
        addAndMakeVisible (customiseTheme);

        themes->addChangeListener (this);
        showThemes();
    }

    ~SettingsPanel() override
    {
        themes->removeChangeListener (this);
        processorRef.treeState.state.removeListener (this);
    }

    FxOrderList& getFxOrder() noexcept { return fxOrder; }

    void paint (juce::Graphics& g) override
    {
        const auto box = getLocalBounds().reduced (Panel::boxInset).toFloat();

        g.setColour (theme().box);
        g.fillRoundedRectangle (box, Panel::boxCornerSize);

        if (theme().boxBorders)
        {
            g.setColour (theme().boxBorder);
            g.drawRoundedRectangle (box.reduced (0.5f), Panel::boxCornerSize, 1.0f);
        }
    }

    void lookAndFeelChanged() override
    {
        for (auto* label : allLabels())
            label->setColour (juce::Label::textColourId, theme().textSettings);
    }

    // a theme dropped in the folder shows up the next time the page opens
    void visibilityChanged() override
    {
        if (isVisible())
            showThemes();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (Panel::boxPadding + 8);

        title.setBounds (bounds.removeFromTop (30));
        bounds.removeFromTop (8);

        auto tooltipRow = bounds.removeFromTop (rowHeight);
        tooltipTitle.setBounds (tooltipRow.removeFromLeft (tooltipRow.getWidth() * 2 / 5));
        tooltipType.setBounds (tooltipRow);

        bounds.removeFromTop (6);

        auto presetRow = bounds.removeFromTop (rowHeight);
        presetFolderTitle.setBounds (presetRow.removeFromLeft (presetRow.getWidth() * 2 / 5));
        changePresetFolder.setBounds (presetRow);

        bounds.removeFromTop (6);

        auto crossoverRow = bounds.removeFromTop (rowHeight);
        crossoverTitle.setBounds (crossoverRow.removeFromLeft (crossoverRow.getWidth() * 2 / 5));
        crossoverSlope.setBounds (crossoverRow);

        bounds.removeFromTop (16);

        auto orderColumn = bounds.removeFromLeft (bounds.getWidth() / 2);

        fxOrderTitle.setBounds (orderColumn.removeFromTop (rowHeight));
        orderColumn.removeFromTop (4);
        fxOrder.setBounds (orderColumn);

        auto themeColumn = bounds.withTrimmedLeft (16);

        themeTitle.setBounds (themeColumn.removeFromTop (rowHeight));
        themeColumn.removeFromTop (4);
        themeSelector.setBounds (themeColumn.removeFromTop (rowHeight));
        themeColumn.removeFromTop (6);
        customiseTheme.setBounds (themeColumn.removeFromTop (rowHeight));
    }

private:
    std::array<juce::Label*, 6> allLabels() { return { &title, &tooltipTitle, &presetFolderTitle, &crossoverTitle, &fxOrderTitle, &themeTitle }; }

    void showThemes()
    {
        shownThemeId = themes->getSelectedId();
        themeEntries = themes->list();
        themeSelector.clear (juce::dontSendNotification);

        for (int i = 0; i < themeEntries.size(); ++i)
        {
            themeSelector.addItem (themeEntries[i].label, i + 1);

            if (themeEntries[i].id == themes->getSelectedId())
                themeSelector.setSelectedId (i + 1, juce::dontSendNotification);
        }
    }

    // whichever instance or window picked or edited the theme, every open settings page keeps the choice.
    // colour edits keep the same theme picked, and reading every theme file on each move of a picker drag is slow
    void changeListenerCallback (juce::ChangeBroadcaster*) override
    {
        if (themes->getSelectedId() == shownThemeId)
            return;

        showThemes();

        if (auto* settings = processorRef.getAppProperties().appProperties.getUserSettings())
            settings->setValue ("theme", themes->getSelectedId());
    }

    void choosePresetFolder()
    {
        presetFolderChooser = std::make_unique<juce::FileChooser> ("Select a folder to save presets to", Preset::defaultDirectory, "*.*", true);

        const auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories;

        presetFolderChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser)
        {
            const auto directory = chooser.getResult();

            if (!directory.exists())
                return;

            processorRef.getAppProperties().appProperties.getUserSettings()->setValue ("presetFolder", directory.getFullPathName());
            processorRef.getPresetManager().setPresetDirectory (directory);
        });
    }

    void showCrossoverSlope()
    {
        crossoverSlope.setSelectedId (MainRouting::crossoverSlopeFrom (processorRef.treeState.state.getProperty (MainRouting::crossoverSlopeProperty)),
                                      juce::dontSendNotification);
    }

    // a load can come in on any thread, so the menu catches up on the message thread
    void valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property) override
    {
        if (tree == processorRef.treeState.state && property == MainRouting::crossoverSlopeProperty)
            triggerAsyncUpdate();
    }

    void valueTreeRedirected (juce::ValueTree&) override { triggerAsyncUpdate(); }
    void handleAsyncUpdate() override { showCrossoverSlope(); }

    static constexpr int rowHeight = 24;

    AudioPluginAudioProcessor& processorRef;

    juce::Label title, tooltipTitle, presetFolderTitle, crossoverTitle, fxOrderTitle, themeTitle;
    juce::ComboBox tooltipType, crossoverSlope, themeSelector;
    juce::TextButton changePresetFolder { "CHANGE FOLDER" }, customiseTheme { "CUSTOMISE" };
    std::unique_ptr<juce::FileChooser> presetFolderChooser;

    FxOrderList fxOrder;

    juce::SharedResourcePointer<ThemeManager> themes;
    juce::Array<ThemeManager::Entry> themeEntries;
    juce::String shownThemeId;
    std::unique_ptr<ThemeCustomiser> customiser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SettingsPanel)
};
