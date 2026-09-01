#pragma once

#include "juce_core/juce_core.h"
#include "juce_audio_processors/juce_audio_processors.h"

namespace ParamIDs
{
    struct PanelInfo
    {
        juce::String name;
        juce::String attachmentID;
        juce::StringArray categories;

        PanelInfo() : name("default"), attachmentID("default"), categories({"default"}) {}
        PanelInfo(juce::String n, juce::String aID, juce::StringArray categories) : name(n), attachmentID(aID), categories({categories}) {}
    };

    struct ParameterInfo
    {   
        public:
        juce::ParameterID id;
        juce::String paramTooltip;
        
        inline juce::String getParamID() const { return id.getParamID(); }
    };

    static const PanelInfo utilityInfo = {
        "UTILITY", "", juce::StringArray({"default"})};

    static const PanelInfo emphasisInfo = {
        "EMPHASIS", "", juce::StringArray({"default"})};

    static const PanelInfo distortion = {
        "DISTORTION", "", juce::StringArray({"GRILL", "TUBE", "PHASE", "RUBIDIUM", "MATRIX", "TAPE", "SLEW"})};

    static const PanelInfo preDistortionTypes = {
        "PRE-DISTORTION", "", juce::StringArray({"DISPERSER", "GRUNGE"})};

    static const PanelInfo noiseTypes = {
        "NOISE", "", juce::StringArray({"SIZZLE", "EROSION", "BIT", "JEFF", "SIZZLE_OG"})};

    static const PanelInfo dynamics = {
        "DYNAMICS", "", juce::StringArray({"STEREO", "OTT", "MID-SIDE", "TYPE-A"})};

    static const PanelInfo quality = {
        "QUALITY", "", juce::StringArray({"FAST", "STANDARD", "HIGH"})};

    static const ParameterInfo inputGain{{"inputGain", 1},
                                         "Changes gain of audio entering Hamburger"};
    static const ParameterInfo outputGain{{"outputGain", 1},
                                          "Changes gain of audio exiting Hamburger"};
    static const ParameterInfo mix{{"mix", 1},
                                   "Changes dry/wet mix of the entire plugin"};

    static const ParameterInfo stages{{"stages", 2},
                                      "Duplicate the current distortion up to 4 times, flipping phase each time"};
    static const int maxStages = 4;

    static const ParameterInfo emphasisLowGain{{"emphasisLowGain", 1},
                                               "Boost or cut the lows going into the distortion, applies reverse on the way out"};
    static const ParameterInfo emphasisMidGain{{"emphasisMidGain", 1},
                                               "Currently unused"};
    static const ParameterInfo emphasisHighGain{{"emphasisHighGain", 1},
                                                "Boost or cut the highs going into the distortion, applies reverse on the way out"};
    static const ParameterInfo emphasisLowFreq{{"emphasisLowFreq", 1},
                                               "Frequency of the low emphasis filter"};
    static const ParameterInfo emphasisMidFreq{{"emphasisMidFreq", 1},
                                               "Currently unused"};
    static const ParameterInfo emphasisHighFreq{{"emphasisHighFreq", 1},
                                                "Frequency of the high emphasis filter"};

    static const ParameterInfo primaryDistortionType{{"primaryDistortionType", 1},
                                                     "Selects which distortion algorithm the main stage uses"};
    static const ParameterInfo noiseDistortionType{{"noiseDistortionType", 1},
                                                   "Selects which noise/degradation algorithm to use"};
    static const ParameterInfo compressionType{{"compressionType", 1},
                                               "Selects which compressor to use in the dynamics stage"};
    static const ParameterInfo compressionOn{{"compressionOn", 1},
                                             "Enables the dynamics stage"};
    static const ParameterInfo emphasisOn{{"emphasisOn", 1},
                                          "Enables the emphasis EQ around the distortion"};
    static const ParameterInfo preDistortionEnabled{{"preDistortionEnabled", 1},
                                                    "Enables the pre-distortion stage"};
    static const ParameterInfo preDistortionType{{"preDistortionType", 2},
                                                 "Selects which pre-distortion algorithm to use"};
    static const ParameterInfo primaryDistortionEnabled{{"primaryDistortionEnabled", 1},
                                                        "Enables the main distortion stage"};
    static const ParameterInfo noiseDistortionEnabled{{"noiseDistortionEnabled", 1},
                                                      "Enables the noise stage"};
    static const ParameterInfo postClipEnabled{{"postClipEnabled", 1},
                                               "Enables the soft clipper on the output"};
    static const ParameterInfo hamburgerEnabled{{"hamburgerEnabled", 1},
                                                "Bypasses the entire plugin when off"};

    static const ParameterInfo oversamplingFactor{{"oversamplingFactor", 1},
                                                  "Upsample signal internally for reduced aliasing. Very CPU expensive at higher values."};

    // compressor
    static const ParameterInfo compSpeed{{"compSpeed", 1},
                                         "Macro for attack + release parameters. Setting to 0ms will turn compressor into a clipper / saturator, instantly applying gain reduction."};
    static const ParameterInfo MBCompSpeed{{"MBCompSpeed", 1},
                                         "Macro for attack + release parameters. Setting to 0ms will turn compressor into a clipper / saturator, instantly applying gain reduction."};
    static const ParameterInfo MSCompSpeed{{"MSCompSpeed", 1},
                                         "Macro for attack + release parameters. Setting to 0ms will turn compressor into a clipper / saturator, instantly applying gain reduction."};
    static const ParameterInfo TypeACompSpeed{{"TypeACompSpeed", 1},
                                         "Macro for attack + release parameters. Setting to 0ms will turn compressor into a clipper / saturator, instantly applying gain reduction."};

    static const ParameterInfo compBandTilt{{"compBandTilt", 1},
                                            "Tilts the threshold across bands, compressing lows or highs harder"};
    static const ParameterInfo compStereoLink{{"compStereoLink", 1},
                                              "How much the left and right channels share gain reduction"};
    static const ParameterInfo compRatio{{"compRatio", 1},
                                         "How hard the signal is compressed once past the threshold"};
    static const ParameterInfo compOut{{"compOut", 1},
                                       "Makeup gain applied after compression"};

    static const ParameterInfo stereoCompThreshold{{"stereoCompThreshold", 1},
                                                   "Level at which the stereo compressor starts working"};
    static const ParameterInfo MBCompThreshold{{"MBCompThreshold", 1},
                                               "Level at which the multiband compressor starts working"};
    static const ParameterInfo MSCompThreshold{{"MSCompThreshold", 1},
                                               "Level at which the mid/side compressor starts working"};

    static const ParameterInfo TypeAThreshold{{"TypeAThreshold", 2},
                                              "Level at which the Type-A compander starts working"};
    static const ParameterInfo TypeARatio{{"TypeARatio", 2},
                                          "How hard the Type-A compander squashes past the threshold"};
    static const ParameterInfo TypeATilt{{"TypeATilt", 2},
                                         "Tilts the Type-A output gain across bands, favouring lows or highs respectively"};
    static const ParameterInfo TypeAOut{{"TypeAOut", 2},
                                        "Output gain applied after the Type-A compander"};

    // gate (noise distortion)
    static const ParameterInfo gateAmt{{"gateAmt", 1},
                                       "The minimum threshold at which gate will let audio pass through"};
    static const ParameterInfo gateMix{{"gateMix", 1},
                                       "Blends the gated signal with the dry signal"};

    // grunge
    static const ParameterInfo grungeAmt{{"grungeAmt", 1},
                                         "Strength of grunge distortion applied"};
    static const ParameterInfo grungeTone{{"grungeTone", 1},
                                          "Chooses suitable frequency to resonate at"};

    // clipper
    static const ParameterInfo postClipGain{{"postClipGain", 1},
                                            "The gain of the audio applied into the clipper"};
    static const ParameterInfo postClipKnee{{"postClipKnee", 1},
                                            "The soft knee width of the clipper before it hits 0db"};

    // grill saturation
    static const ParameterInfo saturationAmount{{"saturationAmount", 1},
                                                "Grill Saturation Strength, all-rounder utility distortion with foldback properties at large amplitudes."};
    static const ParameterInfo diode{{"diode", 1},
                                     "Blends in a waveshape derived from Serum's diode distortion"};
    static const ParameterInfo fold{{"fold", 1},
                                    "Wavefolding, reflects the signal once it goes over 0db, creating strong higher order harmonics"};
    static const ParameterInfo grillBias{{"grillBias", 1},
                                         "Strength of DC offset modulation before the distortion, adding even harmonics"};

    // matrix distortion
    static const ParameterInfo matrix1{{"matrix1", 1},
                                       "Drive into the matrix waveshaper"};
    static const ParameterInfo matrix2{{"matrix2", 1},
                                       "Add gritty ripple to the wave"};
    static const ParameterInfo matrix3{{"matrix3", 1},
                                       "Bounce / Wavefold"};
    static const ParameterInfo matrix4{{"matrix4", 1},
                                       "Crushing / digital stepping"};
    static const ParameterInfo matrix5{{"matrix5", 1},
                                       "Morphs between the filter shapes feeding the waveshaper"};
    static const ParameterInfo matrix6{{"matrix6", 1},
                                       "Cutoff frequency of the matrix filters"};
    static const ParameterInfo matrix7{{"matrix7", 1},
                                       "Currently unused"};
    static const ParameterInfo matrix8{{"matrix8", 1},
                                       "Currently unused"};
    static const ParameterInfo matrix9{{"matrix9", 1},
                                       "Blends the filtered signal against the dry one before shaping"};

    // rubidium
    static const ParameterInfo rubidiumAmount{{"rubidiumAmount", 1},
                                              "Rubidium Strength, unique distortion that adds crest factor to signal"};
    static const ParameterInfo rubidiumMojo{{"rubidiumMojo", 1},
                                            "Increases crest factor of signal"};
    static const ParameterInfo rubidiumAsym{{"rubidiumAsym", 1},
                                            "Makes the crest factor shape asymmetrical around zero crossings"};
    static const ParameterInfo rubidiumTone{{"rubidiumTone", 1},
                                            "Effectively a high pass, more easily increase crest factor increase to the signal for higher frequencies"};
    static const ParameterInfo rubidiumBias{{"rubidiumBias", 2},
                                            "Applies DC offset before the distortion, creating significant even harmonics"};

    // phase distortion
    static const ParameterInfo phaseAmount{{"phaseAmount", 1},
                                           "Phase Distortion Strength, self modulating signal modulating a tiny delay line"};
    static const ParameterInfo phaseDistTone{{"phaseDistTone", 1},
                                             "Low pass filter, exclude frequencies above this from self-modulating"};
    static const ParameterInfo phaseDistStereo{{"phaseDistStereo", 1},
                                               "Make L and R channels self-modulate independently. By default, both channels are summed before modulating."};
    static const ParameterInfo phaseRectify{{"phaseRectify", 1},
                                            "Rectifies the waveform in the modulation path, causing the resulting phase distortion to contain odd harmonics only"};
    static const ParameterInfo phaseShift{{"phaseShift", 1},
                                          "Frequency shift the modulation path. Useful at small values for additional movement"};

    // tube distortion
    static const ParameterInfo tubeAmount{{"tubeAmount", 1},
                                          "Tube Saturation Strength, based on an empirical emulation of Class A tube distortion by Will Pirkle"};
    static const ParameterInfo tubeTone{{"tubeTone", 1},
                                        "Brightness of the tube"};
    static const ParameterInfo tubeBias{{"tubeBias", 1},
                                        "Offsets the waveform into the tube curve, adding even harmonics"};
    static const ParameterInfo jeffAmount{{"jeffAmount", 1},
                                          "Adds an extra aggressive sine waveshaping on top of the signal"};

    // tape hysteresis
    static const ParameterInfo tapeBias{{"tapeBias", 2},
                                        "Applies DC biasing to the signal before distortion, adding even harmonics"};
    static const ParameterInfo tapeDrive{{"tapeDrive", 2},
                                         "Tape Saturation Strength, based on a realistic magnetic tape hysteresis model"};
    static const ParameterInfo tapeWidth{{"tapeWidth", 2},
                                         "Ages the tape, making low end subtly more pronounced by widening the hysteresis curve"};

    // sizzle
    static const ParameterInfo sizzleAmount{{"sizzleAmount", 1},
                                            "Sizzle Noise strength, adds noise in zero crossings of the signal."};
    static const ParameterInfo sizzleFrequency{{"sizzleFrequency", 1},
                                               "Frequency the sizzle noise sits around"};
    static const ParameterInfo sizzleQ{{"sizzleQ", 1},
                                       "Size of sizzle's amplitude band width around the zero crossing"};

    static const ParameterInfo fizzAmount{{"fizzAmount", 1},
                                          "Amount of noise added to the signal. Noise folds back past halfway"};

    // erosion
    static const ParameterInfo erosionAmount{{"erosionAmount", 1},
                                             "Erosion noise strength. Applies noise similar to Ableton's erosion device"};
    static const ParameterInfo erosionFrequency{{"erosionFrequency", 1},
                                                "Frequency the erosion noise resonates at."};
    static const ParameterInfo erosionQ{{"erosionQ", 1},
                                        "Erosion Filter Q factor, resonance amount"};

    static const ParameterInfo downsampleFreq{{"downsampleFreq", 1},
                                              "The new sample rate the signal is crushed to"};
    static const ParameterInfo downsampleMix{{"downsampleMix", 1},
                                             "Dry / Wet blend the digitised and original signal"};
    static const ParameterInfo bitReduction{{"bitReduction", 1},
                                            "Bit depth the signal is quantised to"};

    // allpass
    static const ParameterInfo allPassFreq{{"allPassFreq", 1},
                                           "The frequency the allpass filters center around"};
    static const ParameterInfo allPassQ{{"allPassQ", 1},
                                        "Resonance of the allpass filters"};
    static const ParameterInfo allPassAmount{{"allPassAmount", 1},
                                             "# of Allpasses in the stack, higher number results in transient smearing"};

    // slew
    static const ParameterInfo alphaParam{{"alphaParam", 3},
                                          "Slew distortion strength, a model that modifies \"rate of change\" of signal. If you can't hear changes, set this to max."};
    static const ParameterInfo slewSpeed{{"slewSpeed", 3},
                                         "Slew speed, controls maximum speed that signal can track onto, acting as a fake lowpass control"};
    static const ParameterInfo directionality{{"directionality", 3},
                                              "Bias the slew limiting towards rising or falling edges"};
    static const ParameterInfo slewType{{"slewType", 3},
                                        "Select a slew limiting algorithm to use. Non-standard slew algorithms are present after the first one."};
}