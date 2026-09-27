#pragma once

#include "../PluginProcessor.h"
#include "juce_gui_extra/juce_gui_extra.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "juce_core/juce_core.h"
#include "juce_audio_processors/juce_audio_processors.h"

#include "BinaryData.h"

#include "Info.h"
#include "LeftColumn.h"
#include "SaturationColumn.h"
#include "UtilColumn.h"
#include "SettingsPanel.h"

#include "PresetPanel.h"
#include "PluginHeader.h"
#include "PluginFooter.h"
#include "TooltipBar.h"
#include "UpdateChecker.h"

#include "LookAndFeel/HamburgerLAF.h"
#include "LookAndFeel/ThemeManager.h"

class EditorV2 : public juce::Component, public juce::ChangeListener
{
public:
    // every module's size comes from the columns' height. the start page has fewer and simpler boxes, so it's shorter
    static constexpr int baseWidth = 800;
    static constexpr int columnsHeight = 500;
    static constexpr int startColumnsHeight = 300;
    static constexpr int fullHeight = PluginHeader::totalHeight + columnsHeight + PluginFooter::totalHeight;
    static constexpr int startHeight = PluginHeader::totalHeight + startColumnsHeight + TooltipBar::height + PluginFooter::totalHeight;

    // the start page's scope sits over a row of amounts, as wide as it needs to keep the shape of what it draws in the full view
    static constexpr int scopeMargin = Panel::boxPadding * 2;
    static constexpr int startScopeHeight = startColumnsHeight - UtilColumn::startAmountsHeight;
    static constexpr int startScopeWidth = scopeMargin + (startScopeHeight - scopeMargin) * (baseWidth / 4 - scopeMargin) / (columnsHeight / 4 - scopeMargin);
    static constexpr int startMiddleWidth = 320;
    static constexpr int startWidth = startMiddleWidth + startScopeWidth;

    // when a page needs the window a different size
    std::function<void (int, int)> onSizeChanged;

    // the page picked in the settings, once the window can follow its size
    void openStartupPage()
    {
        const auto page = audioProcessorRef.getAppProperties().getStartupPage();
        header.selectTab(page == "start" ? PluginHeader::Tab::start
                         : page == "pre" ? PluginHeader::Tab::pre
                         : page == "post" ? PluginHeader::Tab::post
                                          : PluginHeader::Tab::main);
    }

    EditorV2(AudioPluginAudioProcessor &p) : audioProcessorRef(p),
                                             leftColumn(p),
                                             saturationColumn(p),
                                             utilColumn(p),
                                             settingsPanel(p),
                                             infoPanel(p)
                                             ,presetPanel(p.getPresetManager()),
                                             header(p),
                                             footer(p)
    {   
        // the first editor opened picks up the theme from last time, any others share it
        if (themes->getSelectedId().isEmpty() && ! themes->select(p.getAppProperties().appProperties.getUserSettings()->getValue("theme", ThemeManager::defaultId)))
            themes->select(ThemeManager::defaultId);

        themes->addChangeListener(this);

        setLookAndFeel(&hamburgerLAF);
        infoPanel.setLookAndFeel(&hamburgerLAF);
        leftColumn.setLookAndFeel(&hamburgerLAF);
        saturationColumn.setLookAndFeel(&hamburgerLAF);
        utilColumn.setLookAndFeel(&hamburgerLAF);
        settingsPanel.setLookAndFeel(&hamburgerLAF);
        presetPanel.setLookAndFeel(&hamburgerLAF);
        footer.setLookAndFeel(&hamburgerLAF);

        addAndMakeVisible(leftColumn);
        addAndMakeVisible(saturationColumn);
        addAndMakeVisible(utilColumn);
        addChildComponent(settingsPanel);
        addChildComponent(settingsSpare);
        addAndMakeVisible(infoPanel);
        addAndMakeVisible(header);
        addAndMakeVisible(footer);
        addChildComponent(tooltipBar);
        header.onTabSelected = [this] (PluginHeader::Tab tab)
        {
            startPage = tab == PluginHeader::Tab::start;
            settingsOpen = tab == PluginHeader::Tab::settings;

            // settings keeps the logo's tooltips and the scope, and takes everything under them in place of the columns
            leftColumn.setVisible (! startPage);
            leftColumn.setLogoOnly (settingsOpen);
            saturationColumn.setVisible (! settingsOpen);
            settingsPanel.setVisible (settingsOpen);
            settingsSpare.setVisible (settingsOpen);
            tooltipBar.setVisible (startPage);
            utilColumn.setLayout (startPage ? UtilColumn::Layout::start : settingsOpen ? UtilColumn::Layout::scopeOnly : UtilColumn::Layout::full);
            saturationColumn.setView (tab);

            if (onSizeChanged != nullptr)
                onSizeChanged (startPage ? startWidth : baseWidth, startPage ? startHeight : fullHeight);

            // settings and the other full size views share a window size, so nothing else would lay them out again
            resized();
        };

        // the fx order names each module in its box's colour
        settingsPanel.getFxOrder().colourFor = [this] (ModuleId id) {
            for (auto* module : { leftColumn.moduleFor (id), saturationColumn.moduleFor (id), utilColumn.moduleFor (id) })
                if (module != nullptr)
                    return (theme().*module->getAccent()).main;

            // the clipper lives in the footer rather than a box
            return id == ModuleId::postClip ? theme().footer.main : theme().plain.main;
        };
        addAndMakeVisible(presetPanel);

        if (audioProcessorRef.getAppProperties().getTooltipType() == AppProperties::TooltipType::window) {
            createTooltipWindow();
        }

        setOpaque(true);

        // for 1 - 4 quick selection
        setWantsKeyboardFocus(true);

        infoPanel.setVisible(false);

        setPaintingIsUnclipped(true);

        p.getAppProperties().addChangeListener(this);

        updater = std::make_unique<UpdateChecker>(JucePlugin_VersionString); // change here to something random to test update mechanism

        updater->shouldCheckForUpdates = [this]() 
        {
            auto* props = audioProcessorRef.getAppProperties().appProperties.getUserSettings();
            if (props == nullptr) return true;

            if (!props->getBoolValue("check_for_updates", true))
                return false;

            juce::int64 lastCancelTime = props->getDoubleValue("last_update_cancel_time", 0.0);
            juce::int64 currentTime = juce::Time::getCurrentTime().toMilliseconds();
            juce::int64 twentyFourHoursInMs = 24LL * 60LL * 60LL * 1000LL;

            if (currentTime - lastCancelTime < twentyFourHoursInMs)
            {
                return false;
            }

            return true;
        };

        updater->onCancelUpdates = [this]() 
        {
            auto* props = audioProcessorRef.getAppProperties().appProperties.getUserSettings();
            if (props != nullptr)
            {
                props->setValue("last_update_cancel_time", 
                                (double)juce::Time::getCurrentTime().toMilliseconds());
                props->saveIfNeeded();
            }
        };

        updater->onDisableUpdates = [this]() 
        {
            auto* props = audioProcessorRef.getAppProperties().appProperties.getUserSettings();
            if (props != nullptr)
            {
                props->setValue("check_for_updates", false);
                props->saveIfNeeded();
            }
        };

        updater->checkForUpdates();

    }

    void createTooltipWindow() {
        tooltipWindow = std::make_unique<juce::TooltipWindow>(this, 600);
        tooltipWindow->setLookAndFeel(&hamburgerLAF);
        addAndMakeVisible(tooltipWindow.get());
    }

    ~EditorV2()
    {
        setLookAndFeel(nullptr);
        infoPanel.setLookAndFeel(nullptr);
        leftColumn.setLookAndFeel(nullptr);
        saturationColumn.setLookAndFeel(nullptr);
        utilColumn.setLookAndFeel(nullptr);
        settingsPanel.setLookAndFeel(nullptr);
        presetPanel.setLookAndFeel(nullptr);
        footer.setLookAndFeel(nullptr);
        if (tooltipWindow != nullptr) {
            tooltipWindow->setLookAndFeel(nullptr);
        }

        audioProcessorRef.getAppProperties().removeChangeListener(this);
        themes->removeChangeListener(this);
    }

    void changeListenerCallback (juce::ChangeBroadcaster* source) override {
        // every component reads its colours from theme() again, and repaints
        if (source == &themes.get())
            sendLookAndFeelChange();

        if (source == &audioProcessorRef.getAppProperties()) {
            bool displayTooltips = audioProcessorRef.getAppProperties().getTooltipType() == AppProperties::TooltipType::window;

            if (displayTooltips && tooltipWindow == nullptr) {
                createTooltipWindow();
            } else {
                tooltipWindow = nullptr;
            }
        }
    }

    void lookAndFeelChanged() override
    {
        hamburgerLAF.applyTheme();

        // drawing it is the slow part, so only when the themed svg comes out different
        auto svg = themes->backgroundSvg();
        applyThemeToSvg(*svg);

        if (const auto themed = svg->toString(); themed != backgroundSvg)
        {
            backgroundSvg = themed;
            background = {};
        }
    }

    // the svg is drawn once into an image, and again only when the theme or the scale changes
    void paint(juce::Graphics &g) override
    {
        const auto scale = g.getInternalContext().getPhysicalPixelScaleFactor();

        if (! background.isValid() || ! juce::approximatelyEqual(scale, backgroundScale))
        {
            backgroundScale = scale;
            background = makeBackground(backgroundSvg, juce::roundToInt((float) getWidth() * scale), juce::roundToInt((float) getHeight() * scale));
        }

        g.drawImage(background, getLocalBounds().toFloat());
    }

    // pressing 1-4 will switch positions on the main editor if we're in a multiband view or similar
    bool keyPressed(const juce::KeyPress &key) override
    {
        const auto character = key.getTextCharacter();

        if (key.getModifiers().isAnyModifierKeyDown() || character < '1' || character > '4')
            return false;

        saturationColumn.selectPosition((int) (character - '1'));
        return true;
    }

    void resized() override
    {
        // drawn at the editor's size, which the start page changes
        background = {};

        auto bounds = getLocalBounds();

        if (tooltipWindow != nullptr) {
            tooltipWindow->setBounds(bounds);
        }

        infoPanel.setBounds(bounds);

        header.setBounds(bounds.removeFromTop(PluginHeader::totalHeight));
        footer.setBounds(bounds.removeFromBottom(PluginFooter::totalHeight));

        // the start page's tooltips run between the header and the boxes, the whole width
        if (startPage)
            tooltipBar.setBounds(bounds.removeFromTop(TooltipBar::height));

        presetPanel.setBounds(getLocalBounds());
        presetPanel.setReservedRight(header.getRightReserved());

        // the logo and the scope where they always are with the spare box between them, and settings across everything under them
        if (settingsOpen)
        {
            auto top = bounds.removeFromTop(columnsHeight / 4);
            leftColumn.setBounds(top.removeFromLeft(baseWidth / 4));
            utilColumn.setBounds(top.removeFromRight(baseWidth / 4));
            settingsSpare.setBounds(top);
            settingsPanel.setBounds(bounds);
            return;
        }

        auto left = bounds.removeFromLeft(startPage ? 0 : baseWidth / 4);
        auto right = bounds.removeFromRight(startPage ? startScopeWidth : baseWidth / 4);

        leftColumn.setBounds(left);
        saturationColumn.setBounds(bounds);
        utilColumn.setBounds(right);
    }

    void handleCommandMessage(int command) override
    {
        bool show = command == 1;

        infoPanel.setVisible(!show);
        
        leftColumn.setVisible(show && ! startPage);
        saturationColumn.setVisible(show && ! settingsOpen);
        utilColumn.setVisible(show);
        settingsPanel.setVisible(show && settingsOpen);
        settingsSpare.setVisible(show && settingsOpen);
        header.setVisible(show);
        footer.setVisible(show);
        tooltipBar.setVisible(show && startPage);
        presetPanel.setVisible(show);
    }

private:
    // an empty black box beside the scope on the settings page, kept for whatever goes there
    struct SpareBox : juce::Component
    {
        void paint(juce::Graphics& g) override
        {
            g.setColour(juce::Colours::black);
            g.fillRoundedRectangle(getLocalBounds().reduced(Panel::boxInset).toFloat(), Panel::boxCornerSize);
        }
    };

    static juce::Image makeBackground(const juce::String& svg, int width, int height)
    {
        auto drawable = juce::Drawable::createFromSVG(*juce::parseXML(svg));

        // shapes can run well past the svg's canvas, so it's the canvas that's fitted rather than the drawing's bounds
        const auto canvas = dynamic_cast<juce::DrawableComposite&>(*drawable).getContentArea();

        juce::Image image(juce::Image::RGB, width, height, true);
        juce::Graphics g(image);

        drawable->draw(g, 1.0f, juce::RectanglePlacement(juce::RectanglePlacement::fillDestination).getTransformToFit(canvas, image.getBounds().toFloat()));
        return image;
    }

    AudioPluginAudioProcessor& audioProcessorRef;
    juce::SharedResourcePointer<ThemeManager> themes;

    LeftColumn leftColumn;
    SaturationColumn saturationColumn;
    UtilColumn utilColumn;
    SettingsPanel settingsPanel;
    SpareBox settingsSpare;

    HamburgerLAF hamburgerLAF;

    std::unique_ptr<juce::TooltipWindow> tooltipWindow;

    PresetPanel presetPanel;
    PluginHeader header;
    PluginFooter footer;
    TooltipBar tooltipBar;
    std::unique_ptr<UpdateChecker> updater;

    Info infoPanel;

    bool startPage = false;
    bool settingsOpen = false;

    juce::Image background;
    float backgroundScale = 0.0f;
    juce::String backgroundSvg;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditorV2)
};