#pragma once
 

#include "../Panel.h"

struct PanelInfo {
    juce::String name;
    juce::String attachmentID;
    juce::StringArray categories;

    PanelInfo() : name("default"), attachmentID("default"), categories({ "default" }) {}
    PanelInfo(juce::String n, juce::String aID, StringArray categories) : name(n), attachmentID(aID), categories({ categories }) {}
};

namespace params {
    static const PanelInfo oversamplingFactor = {
        "OVERSAMPLING", "oversamplingFactor", StringArray({"1x","2x","4x","8x","16x"})
    };

    static const PanelInfo utilityInfo = {
        "UTILITY", "", StringArray({"default"})
    };

    static const PanelInfo emphasisInfo = {
        "EMPHASIS", "", StringArray({"default"})
    };

    static const PanelInfo distortion = {
        "DISTORTION", "", StringArray({"GRILL", "TUBE", "PHASE", "RUBIDIUM"})
    };

    static const PanelInfo preDistortionTypes = {
        "PRE-DISTORTION", "", StringArray({"DISPERSER"})
    };

    static const PanelInfo noiseTypes = {
        "NOISE", "", StringArray({"SIZZLE", "EROSION", "BIT", "JEFF", "SIZZLE_OG"})
    };

    static const PanelInfo dynamics = {
        "DYNAMICS", "", StringArray({"STEREO", "OTT", "MID-SIDE"})
    };

    static const PanelInfo quality = {
        "QUALITY", "", StringArray({"FAST", "STANDARD", "HIGH"})
    };
}
