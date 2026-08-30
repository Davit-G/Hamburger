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

    juce::ApplicationProperties appProperties;
};
