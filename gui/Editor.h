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

#include "PresetPanel.h"
#include "HeaderTabs.h"
#include "SettingsPage.h"
#include "UpdateChecker.h"

#include "LookAndFeel/HamburgerLAF.h"

class EditorV2 : public juce::Component, public juce::ChangeListener
{
public:
    EditorV2(AudioPluginAudioProcessor &p) : audioProcessorRef(p),
                                             leftColumn(p),
                                             saturationColumn(p),
                                             utilColumn(p),
                                             settingsPage(p),
                                             presetPanel(p.getPresetManager())
    {   
        setLookAndFeel(&hamburgerLAF);
        infoPanel.setLookAndFeel(&hamburgerLAF);
        leftColumn.setLookAndFeel(&hamburgerLAF);
        saturationColumn.setLookAndFeel(&hamburgerLAF);
        utilColumn.setLookAndFeel(&hamburgerLAF);

        addAndMakeVisible(leftColumn);
        addAndMakeVisible(saturationColumn);
        addAndMakeVisible(utilColumn);
        addChildComponent(settingsPage);
        addAndMakeVisible(infoPanel);
        addAndMakeVisible(header);
        addAndMakeVisible(presetPanel);

        header.onTabSelected = [this](HeaderTabs::Tab tab) {
            if (tab != HeaderTabs::settings)
                saturationColumn.setView(tab);

            showPage();
        };

        header.typeColour = [this](HeaderTabs::Tab tab) { return saturationColumn.getDistortion(tab).getAccent(); };

        for (auto tab : { HeaderTabs::pre, HeaderTabs::post })
            saturationColumn.getDistortion(tab).onScreenChanged = [this] { header.repaint(); };

        settingsPage.getFxOrder().colourFor = [this](ModuleId id) {
            for (auto* module : { leftColumn.moduleFor(id), saturationColumn.moduleFor(id), utilColumn.moduleFor(id) })
                if (module != nullptr)
                    return module->getAccent();

            return juce::Colours::white;
        };

        if (audioProcessorRef.getAppProperties().getTooltipType() == AppProperties::TooltipType::window) {
            createTooltipWindow();
        }

        setOpaque(true);

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
        if (tooltipWindow != nullptr) {
            tooltipWindow->setLookAndFeel(nullptr);
        }

        audioProcessorRef.getAppProperties().removeChangeListener(this);
    }

    void changeListenerCallback (juce::ChangeBroadcaster* source) override {
        if (source == &audioProcessorRef.getAppProperties()) {
            bool displayTooltips = audioProcessorRef.getAppProperties().getTooltipType() == AppProperties::TooltipType::window;

            if (displayTooltips && tooltipWindow == nullptr) {
                createTooltipWindow();
            } else {
                tooltipWindow = nullptr;
            }
        }
    }

    void paint(juce::Graphics &g) override
    {
        g.drawImage(image, getLocalBounds().toFloat(), juce::RectanglePlacement::fillDestination);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto totalWidth = bounds.getWidth() / 4;

        if (tooltipWindow != nullptr) {
            tooltipWindow->setBounds(bounds);
        }

        infoPanel.setBounds(bounds);
        header.setBounds(bounds.removeFromTop(HeaderTabs::height));

        presetPanel.barLeft = header.getLeftReserved();
        presetPanel.barRight = header.getRightReserved();
        presetPanel.setBounds(getLocalBounds());

        settingsPage.setBounds(bounds);

        auto left = bounds.removeFromLeft(totalWidth);
        auto right = bounds.removeFromRight(totalWidth);

        leftColumn.setBounds(left);
        saturationColumn.setBounds(bounds);
        utilColumn.setBounds(right);
    }

    // 0 from clicking the logo opens the info page over everything, 1 from clicking the info page closes it
    void handleCommandMessage(int command) override
    {
        infoOpen = command == 0;
        showPage();
    }

private:
    void showPage()
    {
        const bool settings = header.getSelected() == HeaderTabs::settings;

        infoPanel.setVisible(infoOpen);
        header.setVisible(!infoOpen);
        presetPanel.setVisible(!infoOpen);
        leftColumn.setVisible(!infoOpen && !settings);
        saturationColumn.setVisible(!infoOpen && !settings);
        utilColumn.setVisible(!infoOpen && !settings);
        settingsPage.setVisible(!infoOpen && settings);
    }

    AudioPluginAudioProcessor& audioProcessorRef;

    LeftColumn leftColumn;
    SaturationColumn saturationColumn;
    UtilColumn utilColumn;

    SettingsPage settingsPage;

    HamburgerLAF hamburgerLAF;

    std::unique_ptr<juce::TooltipWindow> tooltipWindow;

    HeaderTabs header;
    PresetPanel presetPanel;
    std::unique_ptr<UpdateChecker> updater;

    Info infoPanel;
    bool infoOpen = false;

    juce::Image image = juce::ImageCache::getFromMemory(BinaryData::bg4_jpg, BinaryData::bg4_jpgSize);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditorV2)
};