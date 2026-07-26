#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

class AppProperties {
    public:
    AppProperties() {
        auto options = juce::PropertiesFile::Options();
        
        options.applicationName = JucePlugin_Name;
        options.filenameSuffix = ".settings";
        options.osxLibrarySubFolder = "Application Support/AviaryAudio";
        options.folderName = juce::String(JucePlugin_Manufacturer) + "/" + juce::String(JucePlugin_Name);
        options.storageFormat = juce::PropertiesFile::storeAsXML;

        appProperties.setStorageParameters(options);
    }
    
    juce::ApplicationProperties appProperties;
};