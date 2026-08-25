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
#include "UpdateChecker.h"

#include "LookAndFeel/HamburgerLAF.h"

class EditorV2 : public juce::AudioProcessorEditor
{
public:
    EditorV2(AudioPluginAudioProcessor &p) : AudioProcessorEditor(&p),
                                             audioProcessorRef(p),
                                             leftColumn(p),
                                             saturationColumn(p),
                                             utilColumn(p),
                                             infoPanel(p)
                                             ,presetPanel(p.getPresetManager())
    {
        int additionalHeight = 0;

        setLookAndFeel(&hamburgerLAF);
        infoPanel.setLookAndFeel(&hamburgerLAF);
        leftColumn.setLookAndFeel(&hamburgerLAF);
        saturationColumn.setLookAndFeel(&hamburgerLAF);
        utilColumn.setLookAndFeel(&hamburgerLAF);

        // setResizable(true, true);
        // setResizeLimits(500, 300, 1200, 700);

        

        addAndMakeVisible(leftColumn);
        addAndMakeVisible(saturationColumn);
        addAndMakeVisible(utilColumn);
        addAndMakeVisible(infoPanel);
        addAndMakeVisible(presetPanel);

        additionalHeight += 45;

        setOpaque(true);

        infoPanel.setVisible(false);

        setPaintingIsUnclipped(true);

        setSize(800, 500 + additionalHeight);

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

    ~EditorV2()
    {
        setLookAndFeel(nullptr);
        infoPanel.setLookAndFeel(nullptr);
        leftColumn.setLookAndFeel(nullptr);
        saturationColumn.setLookAndFeel(nullptr);
        utilColumn.setLookAndFeel(nullptr);
    }

    void paint(juce::Graphics &g) override
    {
        g.drawImage(image, getLocalBounds().toFloat(), juce::RectanglePlacement::fillDestination);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto totalWidth = bounds.getWidth() / 4;

        infoPanel.setBounds(bounds);

        presetPanel.setBounds(bounds);
        bounds.removeFromTop(45);

        // this is where I would add a panel for switching between screens
        

        auto left = bounds.removeFromLeft(totalWidth);
        auto right = bounds.removeFromRight(totalWidth);

        leftColumn.setBounds(left);
        saturationColumn.setBounds(bounds);
        utilColumn.setBounds(right);
    }

    void handleCommandMessage(int command) override
    {
        bool show = command == 1;

        infoPanel.setVisible(!show);
        
        leftColumn.setVisible(show);
        saturationColumn.setVisible(show);
        utilColumn.setVisible(show);
        presetPanel.setVisible(show);
    }

private:
    AudioPluginAudioProcessor& audioProcessorRef;

    LeftColumn leftColumn;
    SaturationColumn saturationColumn;
    UtilColumn utilColumn;

    HamburgerLAF hamburgerLAF;

    PresetPanel presetPanel;
    std::unique_ptr<UpdateChecker> updater;

    Info infoPanel;

    juce::Image image = juce::ImageCache::getFromMemory(BinaryData::bg4_jpg, BinaryData::bg4_jpgSize);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditorV2)
};