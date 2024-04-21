#pragma once

#include "juce_core/juce_core.h"
#include "juce_audio_processors/juce_audio_processors.h"

namespace ParamIDs {
    struct PanelInfo {
        juce::String name;
        juce::String attachmentID;
        juce::StringArray categories;

        PanelInfo() : name("default"), attachmentID("default"), categories({ "default" }) {}
        PanelInfo(juce::String n, juce::String aID, juce::StringArray categories) : name(n), attachmentID(aID), categories({ categories }) {}
    };

    static const PanelInfo utilityInfo = {
        "UTILITY", "", juce::StringArray({"default"})
    };

    static const PanelInfo emphasisInfo = {
        "EMPHASIS", "", juce::StringArray({"default"})
    };

    static const PanelInfo distortion = {
        "DISTORTION", "", juce::StringArray({"GRILL", "TUBE", "PHASE", "RUBIDIUM", "MATRIX"})
    };

    static const PanelInfo preDistortionTypes = {
        "PRE-DISTORTION", "", juce::StringArray({"DISPERSER"})
    };

    static const PanelInfo noiseTypes = {
        "NOISE", "", juce::StringArray({"SIZZLE", "EROSION", "BIT", "JEFF", "SIZZLE_OG"})
    };

    static const PanelInfo dynamics = {
        "DYNAMICS", "", juce::StringArray({"STEREO", "OTT", "MID-SIDE"})
    };

    static const PanelInfo quality = {
        "QUALITY", "", juce::StringArray({"FAST", "STANDARD", "HIGH"})
    };


    static const juce::ParameterID inputGain { "inputGain", 1 };
    static const juce::ParameterID outputGain { "outputGain", 1 };
    static const juce::ParameterID mix { "mix", 1 };


    static const juce::ParameterID emphasisLowGain { "emphasisLowGain", 1 };
    static const juce::ParameterID emphasisMidGain { "emphasisMidGain", 1 };
    static const juce::ParameterID emphasisHighGain { "emphasisHighGain", 1 };
    static const juce::ParameterID emphasisLowFreq { "emphasisLowFreq", 1 };
    static const juce::ParameterID emphasisMidFreq { "emphasisMidFreq", 1 };
    static const juce::ParameterID emphasisHighFreq { "emphasisHighFreq", 1 };


    static const juce::ParameterID primaryDistortionType { "primaryDistortionType", 1 };
    static const juce::ParameterID noiseDistortionType { "noiseDistortionType", 1 };
    static const juce::ParameterID compressionType { "compressionType", 1 };
    static const juce::ParameterID compressionOn { "compressionOn", 1 };
    static const juce::ParameterID emphasisOn { "emphasisOn", 1 };
    static const juce::ParameterID preDistortionEnabled { "preDistortionEnabled", 1 };
    static const juce::ParameterID primaryDistortionEnabled { "primaryDistortionEnabled", 1 };
    static const juce::ParameterID noiseDistortionEnabled { "noiseDistortionEnabled", 1 };
    static const juce::ParameterID postClipEnabled { "postClipEnabled", 1 };
    static const juce::ParameterID hamburgerEnabled { "hamburgerEnabled", 1 };

    static const juce::ParameterID oversamplingFactor { "oversamplingFactor", 1 };

     // compressor
    static const juce::ParameterID compSpeed { "compSpeed", 1 };
    static const juce::ParameterID compBandTilt { "compBandTilt", 1 };
    static const juce::ParameterID compStereoLink { "compStereoLink", 1 };
    static const juce::ParameterID compRatio { "compRatio", 1 };
    static const juce::ParameterID compOut { "compOut", 1 };

    static const juce::ParameterID stereoCompThreshold { "stereoCompThreshold", 1 };
    static const juce::ParameterID MBCompThreshold { "MBCompThreshold", 1 };
    static const juce::ParameterID MSCompThreshold { "MSCompThreshold", 1 };

     // gate (noise distortion)
    static const juce::ParameterID gateAmt { "gateAmt", 1 };
    static const juce::ParameterID gateMix { "gateMix", 1 };

     // grunge
    static const juce::ParameterID grungeAmt { "grungeAmt", 1 };
    static const juce::ParameterID grungeTone { "grungeTone", 1 };

     // clipper
    static const juce::ParameterID postClipGain { "postClipGain", 1 };
    static const juce::ParameterID postClipKnee { "postClipKnee", 1 };

     // grill saturation
    static const juce::ParameterID saturationAmount { "saturationAmount", 1 };
    static const juce::ParameterID diode { "diode", 1 };
    static const juce::ParameterID fold { "fold", 1 };
    static const juce::ParameterID grillBias { "grillBias", 1 };

    // matrix distortion
    static const juce::ParameterID matrix1 { "matrix1", 1 };
    static const juce::ParameterID matrix2 { "matrix2", 1 };
    static const juce::ParameterID matrix3 { "matrix3", 1 };
    static const juce::ParameterID matrix4 { "matrix4", 1 };
    static const juce::ParameterID matrix5 { "matrix5", 1 };
    static const juce::ParameterID matrix6 { "matrix6", 1 };
    static const juce::ParameterID matrix7 { "matrix7", 1 };
    static const juce::ParameterID matrix8 { "matrix8", 1 };
    static const juce::ParameterID matrix9 { "matrix9", 1 };

    // rubidium
    static const juce::ParameterID rubidiumAmount { "rubidiumAmount", 1 };
    static const juce::ParameterID rubidiumMojo { "rubidiumMojo", 1 };
    static const juce::ParameterID rubidiumAsym { "rubidiumAsym", 1 };
    static const juce::ParameterID rubidiumTone { "rubidiumTone", 1 };

    // phase distortion
    static const juce::ParameterID phaseAmount { "phaseAmount", 1 };
    static const juce::ParameterID phaseDistTone { "phaseDistTone", 1 };
    static const juce::ParameterID phaseDistNormalise { "phaseDistNormalise", 1 };

    // tube distortion
    static const juce::ParameterID tubeAmount { "tubeAmount", 1 };
    static const juce::ParameterID tubeTone { "tubeTone", 1 };
    static const juce::ParameterID tubeBias { "tubeBias", 1 };
    static const juce::ParameterID jeffAmount { "jeffAmount", 1 };

    // sizzle
    static const juce::ParameterID sizzleAmount { "sizzleAmount", 1 };
    static const juce::ParameterID sizzleFrequency { "sizzleFrequency", 1 };
    static const juce::ParameterID sizzleQ { "sizzleQ", 1 };

    static const juce::ParameterID fizzAmount { "fizzAmount", 1 };

    // erosion
    static const juce::ParameterID erosionAmount { "erosionAmount", 1 };
    static const juce::ParameterID erosionFrequency { "erosionFrequency", 1 };
    static const juce::ParameterID erosionQ { "erosionQ", 1 };


    static const juce::ParameterID downsampleFreq { "downsampleFreq", 1 };
    static const juce::ParameterID downsampleMix { "downsampleMix", 1 };
    static const juce::ParameterID bitReduction { "bitReduction", 1 };

    // allpass
    static const juce::ParameterID allPassFreq { "allPassFreq", 1 };
    static const juce::ParameterID allPassQ { "allPassQ", 1 };
    static const juce::ParameterID allPassAmount { "allPassAmount", 1 };
}