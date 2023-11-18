#pragma once
#include <JuceHeader.h>

#include "../Panel.h"

struct PanelInfo {
    juce::String name;
    juce::String attachmentID;
    juce::StringArray categories;

    PanelInfo() : name("default"), attachmentID("default"), categories({ "default" }) {}
    PanelInfo(juce::String n, juce::String aID, StringArray categories) : name(n), attachmentID(aID), categories({ categories }) {}
};

// export stringarrays for all the stuff
// StringArray({ "Classic", "Tube" })
// StringArray({ "Disperser", "Reverb", "Comb" })
// StringArray({ "Sizzle", "Erosion", "Asperity", "Downsample / Bitreduction", "Jeff Thickness"})

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

    static const PanelInfo companderInfo = {
        "COMPANDER", "", StringArray({"default"})
    };

    static const PanelInfo distortion = {
        "DISTORTION", "", StringArray({"CLASSIC", "TUBE"})
    };

    static const PanelInfo preDistortionTypes = {
        "PRE-DISTORTION", "", StringArray({"DISPERSER"})
    };

    static const PanelInfo noiseTypes = {
        "NOISE", "", StringArray({"SIZZLE", "EROSION", "REDUCE", "SMOOTHREDUCE", "JEFF"})
    };

    static const PanelInfo dynamics = {
        "DYNAMICS", "", StringArray({"COMPANDER", "OTT", "MID-SIDE"})
    };
};
