#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

class AppProperties : public juce::ChangeBroadcaster
{
public:
    enum class TooltipType { none, window, boxLabel };

    AppProperties() 
    {
        auto options = juce::PropertiesFile::Options();
        
        options.applicationName     = JucePlugin_Name;
        options.filenameSuffix      = ".settings";
        options.osxLibrarySubFolder = "Application Support/AviaryAudio";
        options.folderName          = juce::String (JucePlugin_Manufacturer) + "/" + juce::String (JucePlugin_Name);
        options.storageFormat       = juce::PropertiesFile::storeAsXML;

        appProperties.setStorageParameters (options);
    }
    
    void setTooltipType (TooltipType newType)
    {
        if (getTooltipType() != newType)
        {
            auto* userSettings = appProperties.getUserSettings();
            if (userSettings != nullptr)
            {
                // Save as integer: 0 for window, 1 for boxLabel
                userSettings->setValue ("tooltipType", static_cast<int> (newType));
                userSettings->saveIfNeeded();
                
                sendChangeMessage(); // Broadcast change to components
            }
        }
    }
    
    TooltipType getTooltipType()
    {
        if (auto* userSettings = appProperties.getUserSettings())
        {
            int typeVal = userSettings->getIntValue ("tooltipType", 2);
            return static_cast<TooltipType> (typeVal);
        }
        return TooltipType::window;
    }

    // the page the plugin opens on: start, pre, main or post
    juce::String getStartupPage()
    {
        auto* userSettings = appProperties.getUserSettings();
        return userSettings != nullptr ? userSettings->getValue ("startupPage", "main") : juce::String ("main");
    }

    void setStartupPage (const juce::String& page)
    {
        if (auto* userSettings = appProperties.getUserSettings())
        {
            userSettings->setValue ("startupPage", page);
            userSettings->saveIfNeeded();
        }
    }

    // whether knobs glide to where they're set, off for hosts that already send sample accurate automation
    bool getParamSmoothing()
    {
        auto* userSettings = appProperties.getUserSettings();
        return userSettings == nullptr || userSettings->getBoolValue ("paramSmoothing", true);
    }

    void setParamSmoothing (bool shouldSmooth)
    {
        if (auto* userSettings = appProperties.getUserSettings())
        {
            userSettings->setValue ("paramSmoothing", shouldSmooth);
            userSettings->saveIfNeeded();
        }
    }

    juce::ApplicationProperties appProperties;
};
