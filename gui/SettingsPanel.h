#pragma once

#include "../PluginProcessor.h"
#include "LookAndFeel/HamburgerLAF.h"
#include "LookAndFeel/ThemeManager.h"
#include "Modules/Panel.h"
#include "Knob.h"
#include "ThemeCustomiser.h"
#include "FxOrderList.h"
#include "PluginHeader.h"

class SettingsPanel : public juce::Component,
                      private juce::ValueTree::Listener,
                      private juce::AsyncUpdater,
                      private juce::ChangeListener
{
public:
    explicit SettingsPanel(AudioPluginAudioProcessor& p)
        : processorRef(p), fxOrder(p), oversampling(p, "OVERSAMPLING", ParamIDs::oversamplingFactor)
    {
        for (auto* label : allLabels())
        {
            label->setJustificationType(juce::Justification::centredLeft);
            addAndMakeVisible(label);
        }

        title.setText("SETTINGS", juce::dontSendNotification);
        HamburgerLAF::setLabelFontScale(title, 1.4f);
        HamburgerLAF::setLabelFontScale(fxOrderTitle, 1.4f);

        tooltipTitle.setText("TOOLTIPS", juce::dontSendNotification);
        startupTitle.setText("OPENS ON", juce::dontSendNotification);
        presetFolderTitle.setText("PRESET FOLDER", juce::dontSendNotification);
        crossoverTitle.setText("CROSSOVERS", juce::dontSendNotification);
        highpassTitle.setText("DC BLOCKERS", juce::dontSendNotification);
        dryPhaseTitle.setText("DRY PHASE", juce::dontSendNotification);
        smoothingTitle.setText("SMOOTHING", juce::dontSendNotification);
        fxOrderTitle.setText("FX ORDER", juce::dontSendNotification);
        themeTitle.setText("THEME", juce::dontSendNotification);

        tooltipType.addItem("None", 1);
        tooltipType.addItem("Hovering", 2);
        tooltipType.addItem("Replace Logo", 3);
        tooltipType.setSelectedId(static_cast<int>(processorRef.getAppProperties().getTooltipType()) + 1, juce::dontSendNotification);
        tooltipType.onChange = [this] {
            processorRef.getAppProperties().setTooltipType(static_cast<AppProperties::TooltipType>(tooltipType.getSelectedId() - 1));
        };
        addAndMakeVisible(tooltipType);

        // by the ids AppProperties keeps, in the order they're offered, each id one past its place in startupPages
        for (auto [id, name] : { std::pair { 1, "Start" }, std::pair { 2, "Pre" }, std::pair { 3, "Main" }, std::pair { 4, "Post" } })
            if (id != 1 || PluginHeader::hasStartPage)
                startupPage.addItem(name, id);

        startupPage.setTooltip("The page the plugin opens on");
        startupPage.setSelectedId(startupPages.indexOf(processorRef.getAppProperties().getStartupPage()) + 1, juce::dontSendNotification);

        if (startupPage.getSelectedId() == 0)
            startupPage.setSelectedId(2, juce::dontSendNotification);

        startupPage.onChange = [this] { processorRef.getAppProperties().setStartupPage(startupPages[startupPage.getSelectedId() - 1]); };
        addAndMakeVisible(startupPage);

        smoothing.addItem("On", 1);
        smoothing.addItem("Off", 2);
        smoothing.setTooltip("Glides knobs over 10ms so they don't click. Off for sample accurate hosts");
        smoothing.setSelectedId(processorRef.getAppProperties().getParamSmoothing() ? 1 : 2, juce::dontSendNotification);
        smoothing.onChange = [this] {
            const auto on = smoothing.getSelectedId() == 1;
            processorRef.getAppProperties().setParamSmoothing(on);
            SmoothParam::smoothingEnabled = on;
        };
        addAndMakeVisible(smoothing);

        changePresetFolder.onClick = [this] { choosePresetFolder(); };
        addAndMakeVisible(changePresetFolder);

        // in the plugin state rather than the app's settings, so it saves with the preset or project like the sound does
        for (const auto slope : { 12, 24, 48 })
            crossoverSlope.addItem(juce::String(slope) + " dB/oct", slope);

        crossoverSlope.addItem("Linear phase", MainRouting::linearPhase);

        crossoverSlope.setTooltip("Band split steepness. Linear phase adds 40ms latency. Saved with preset");
        crossoverSlope.onChange = [this] {
            processorRef.treeState.state.setProperty(MainRouting::crossoverSlopeProperty, crossoverSlope.getSelectedId(), nullptr);
        };
        addAndMakeVisible(crossoverSlope);

        // by DCBlockingHighPass::Mode, less one
        dcFilter.addItem("2nd order", 1);
        dcFilter.addItem("1st order", 2);
        dcFilter.setTooltip("DC blocking filters. 1st order rings less in stacks");
        dcFilter.onChange = [this] {
            processorRef.treeState.state.setProperty(MainRouting::highpassModeProperty, dcFilter.getSelectedId() - 1, nullptr);
        };
        addAndMakeVisible(dcFilter);

        dryPhase.addItem("Matched", 1);
        dryPhase.addItem("Off", 2);
        dryPhase.setTooltip("Puts the dry through the crossovers' phase shift, so dry/wet won't comb");
        dryPhase.onChange = [this] {
            processorRef.treeState.state.setProperty(MainRouting::dryPhaseProperty, dryPhase.getSelectedId() == 1, nullptr);
        };
        addAndMakeVisible(dryPhase);

        // the state tree gets swapped whole on a preset or project load, which is valueTreeRedirected
        processorRef.treeState.state.addListener(this);
        showChainSettings();

        addAndMakeVisible(fxOrder);

        oversampling.setAccent(&Theme::plain);
        addAndMakeVisible(oversampling);

        themeSelector.onChange = [this] {
            if (const auto index = themeSelector.getSelectedId() - 1; juce::isPositiveAndBelow(index, themeEntries.size()))
                themes->select(themeEntries[index].id);
        };
        addAndMakeVisible(themeSelector);

        customiseTheme.setTooltip("Opens a window with every colour in the theme");
        customiseTheme.onClick = [this] {
            if (customiser == nullptr)
                customiser = std::make_unique<ThemeCustomiser>();

            customiser->setVisible(true);
            customiser->toFront(true);
        };
        addAndMakeVisible(customiseTheme);

        themes->addChangeListener(this);
        showThemes();
    }

    ~SettingsPanel() override
    {
        themes->removeChangeListener(this);
        processorRef.treeState.state.removeListener(this);
    }

    FxOrderList& getFxOrder() noexcept { return fxOrder; }

    void paint(juce::Graphics& g) override
    {
        const auto box = getLocalBounds().reduced(Panel::boxInset).toFloat();

        g.setColour(theme().box);
        g.fillRoundedRectangle(box, Panel::boxCornerSize);

        if (theme().boxBorders)
        {
            g.setColour(theme().boxBorder);
            g.drawRoundedRectangle(box.reduced(0.5f), Panel::boxCornerSize, 1.0f);
        }
    }

    void lookAndFeelChanged() override
    {
        for (auto* label : allLabels())
            label->setColour(juce::Label::textColourId, theme().labelText);
    }

    // a theme dropped in the folder shows up the next time the page opens
    void visibilityChanged() override
    {
        if (isVisible())
            showThemes();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(Panel::boxPadding + 8);

        // three columns across: the fx order under its title the whole way down, then the general settings and
        // oversampling with the theme and preset folder under the settings title
        const auto columnWidth = (bounds.getWidth() - columnGap * 2) / 3;
        auto orderColumn = bounds.removeFromLeft(columnWidth);
        bounds.removeFromLeft(columnGap);

        fxOrderTitle.setBounds(orderColumn.removeFromTop(30));
        orderColumn.removeFromTop(8);

        title.setBounds(bounds.removeFromTop(30));
        bounds.removeFromTop(8);

        auto general = bounds.removeFromLeft(columnWidth);
        auto themeColumn = bounds.withTrimmedLeft(columnGap);

        for (auto [label, control] : std::initializer_list<std::pair<juce::Label*, juce::Component*>> {
                 { &tooltipTitle, &tooltipType }, { &startupTitle, &startupPage }, { &crossoverTitle, &crossoverSlope },
                 { &highpassTitle, &dcFilter }, { &dryPhaseTitle, &dryPhase }, { &smoothingTitle, &smoothing } })
        {
            auto row = general.removeFromTop(rowHeight);
            label->setBounds(row.removeFromLeft(row.getWidth() * 2 / 5));
            control->setBounds(row);
            general.removeFromTop(6);
        }

        fxOrder.setBounds(orderColumn.removeFromTop(orderColumn.getHeight() * 3 / 4));

        // the whole column across, so its name has room under the round knob
        oversampling.setBounds(themeColumn.removeFromTop(knobHeight));
        themeColumn.removeFromTop(16);

        themeTitle.setBounds(themeColumn.removeFromTop(rowHeight));
        themeColumn.removeFromTop(4);
        themeSelector.setBounds(themeColumn.removeFromTop(rowHeight));
        themeColumn.removeFromTop(6);
        customiseTheme.setBounds(themeColumn.removeFromTop(rowHeight));
        themeColumn.removeFromTop(16);

        presetFolderTitle.setBounds(themeColumn.removeFromTop(rowHeight));
        themeColumn.removeFromTop(4);
        changePresetFolder.setBounds(themeColumn.removeFromTop(rowHeight));
    }

private:
    std::array<juce::Label*, 10> allLabels() { return { &title, &tooltipTitle, &startupTitle, &presetFolderTitle, &crossoverTitle, &highpassTitle, &dryPhaseTitle, &smoothingTitle, &fxOrderTitle, &themeTitle }; }

    void showThemes()
    {
        shownThemeId = themes->getSelectedId();
        themeEntries = themes->list();
        themeSelector.clear(juce::dontSendNotification);

        for (int i = 0; i < themeEntries.size(); ++i)
        {
            themeSelector.addItem(themeEntries[i].label, i + 1);

            if (themeEntries[i].id == themes->getSelectedId())
                themeSelector.setSelectedId(i + 1, juce::dontSendNotification);
        }
    }

    // whichever instance or window picked or edited the theme, every open settings page keeps the choice.
    // colour edits keep the same theme picked, and reading every theme file on each move of a picker drag is slow
    void changeListenerCallback(juce::ChangeBroadcaster*) override
    {
        showScopeViewFor(themes->getHovered());

        if (themes->getSelectedId() == shownThemeId)
            return;

        showThemes();

        if (auto* settings = processorRef.getAppProperties().appProperties.getUserSettings())
            settings->setValue("theme", themes->getSelectedId());
    }

    // hovering a scope colour in the customiser brings up the scope view it's drawn in, until the mouse moves off it
    void showScopeViewFor(const ThemeColour* colour)
    {
        auto& scope = processorRef.getScopeContext();

        if (const auto view = colour != nullptr ? scopeViewFor(colour->id) : std::nullopt)
        {
            scope.setType(*view);
            showingScopeView = true;
        }
        else if (showingScopeView)
        {
            scope.startDecaying();
            showingScopeView = false;
        }
    }

    static std::optional<ScopeContextType> scopeViewFor(const juce::String& id)
    {
        if (id.startsWith("comp_"))                                          return ScopeContextType::COMPRESSION;
        if (id.startsWith("scope_clip_") || id.startsWith("clip_"))        return ScopeContextType::CLIPPER;
        if (id.startsWith("spectrum_") || id.startsWith("scope_curve"))   return ScopeContextType::SPECTRUM_EMPHASIS;
        if (id == "scope_watermark")                                          return ScopeContextType::WATERMARK;
        if (id.startsWith("scope_"))                                         return ScopeContextType::LR_SCOPE;

        return {};
    }

    void choosePresetFolder()
    {
        presetFolderChooser = std::make_unique<juce::FileChooser>("Select a folder to save presets to", Preset::defaultDirectory, "*.*", true);

        const auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories;

        presetFolderChooser->launchAsync(flags, [this](const juce::FileChooser& chooser)
        {
            const auto directory = chooser.getResult();

            if (!directory.exists())
                return;

            processorRef.getAppProperties().appProperties.getUserSettings()->setValue("presetFolder", directory.getFullPathName());
            processorRef.getPresetManager().setPresetDirectory(directory);
        });
    }

    void showChainSettings()
    {
        const auto& state = processorRef.treeState.state;
        crossoverSlope.setSelectedId(MainRouting::crossoverSlopeFrom(state.getProperty(MainRouting::crossoverSlopeProperty)), juce::dontSendNotification);
        dcFilter.setSelectedId(juce::jlimit(0, 1, (int) state.getProperty(MainRouting::highpassModeProperty)) + 1, juce::dontSendNotification);
        dryPhase.setSelectedId((bool) state.getProperty(MainRouting::dryPhaseProperty, true) ? 1 : 2, juce::dontSendNotification);
    }

    // a load can come in on any thread, so the menu catches up on the message thread
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override
    {
        if (tree == processorRef.treeState.state && (property == MainRouting::crossoverSlopeProperty || property == MainRouting::highpassModeProperty
                                                        || property == MainRouting::dryPhaseProperty))
            triggerAsyncUpdate();
    }

    void valueTreeRedirected(juce::ValueTree&) override { triggerAsyncUpdate(); }
    void handleAsyncUpdate() override { showChainSettings(); }

    static constexpr int rowHeight = 24, knobHeight = 84;
    static constexpr int columnGap = 24;

    AudioPluginAudioProcessor& processorRef;

    juce::Label title, tooltipTitle, startupTitle, presetFolderTitle, crossoverTitle, highpassTitle, dryPhaseTitle, smoothingTitle, fxOrderTitle, themeTitle;
    juce::ComboBox tooltipType, startupPage, crossoverSlope, dcFilter, dryPhase, smoothing, themeSelector;
    const juce::StringArray startupPages { "start", "pre", "main", "post" };
    juce::TextButton changePresetFolder { "CHANGE FOLDER" }, customiseTheme { "CUSTOMISE" };
    std::unique_ptr<juce::FileChooser> presetFolderChooser;

    FxOrderList fxOrder;
    ParamKnob oversampling;

    juce::SharedResourcePointer<ThemeManager> themes;
    juce::Array<ThemeManager::Entry> themeEntries;
    juce::String shownThemeId;
    bool showingScopeView = false;
    std::unique_ptr<ThemeCustomiser> customiser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsPanel)
};
