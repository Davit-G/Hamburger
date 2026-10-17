#pragma once

#include "juce_audio_processors/juce_audio_processors.h"

#include "Module.h"
#include "Module.h"
#include "ValueType.h"
#include "ParamRange.h"
#include "Slot.h"

namespace ParamIDs
{
    // number of reserved param slots in the case where it's unused
    static constexpr int choiceSlots = 16;

    // a type menu's names padded out to choiceSlots, so types can be added later without changing the parameter
    inline juce::StringArray withReservedSlots (juce::StringArray names)
    {
        jassert (names.size() <= choiceSlots);

        for (int i = names.size(); i < choiceSlots; ++i)
            names.add ("RESERVED " + juce::String (i + 1));

        return names;
    }

    /*  VERSION HINTS. Audio Units hand hosts the parameters sorted by version hint, then by ID, and Logic and GarageBand
        keep automation by that order: a parameter that sorts in among the released ones moves their automation onto
        the wrong parameters. So every parameter added after a release needs a higher hint than anything released.

        When a version ships, set releasedVersionHint to the highest hint in it and releasedParameterCount to its total
        parameter count, printed in a debug build. After that a debug build asserts on start up if a parameter at a
        released hint was added, or one removed. */
    static constexpr int releasedVersionHint = 1;
    static constexpr int releasedParameterCount = 0; // 0 until the slotted version ships, which leaves the check off

    static const juce::StringArray distortionTypes {"GRILL", "TUBE", "PHASE", "RUBIDIUM", "TAPE", "SLEW", "WAVESHAPE",
#if JUCE_DEBUG
 "DLL" // debug only, always last. hot reloads a dylib, see hotdsp/HotDsp.h
#endif 
    };
    
    static const juce::StringArray compTypes { "STEREO", "MB", "MS", "TYPE A", "TRANSIENT", "MB TRANSIENT", "OPTO", "MB OPTO" };
    static const juce::StringArray noiseTypes {"SIZZLE", "EROSION", "BIT", "GATE", "FIZZ"};
    static const juce::StringArray clipTypes {"CLIP", "LIMIT"};
    static const juce::StringArray preFxTypes {"ALLPASS", "GRUNGE", "SUB GEN", "HILBERT"};
    static const juce::StringArray emphasisTypes {"EQ", "TILT"}; // keep in line with EmphasisFilter::Mode
    static const juce::StringArray routingTypes {"STACK", "MULTIBAND", "MID/SIDE", "EXCITER"}; // keep in step with MainRouting::Routing

    // mapping between module IDs and the categories that each module contains. used for auto param generation
    inline const juce::StringArray* categoriesFor(ModuleId id)
    {
        switch (id)
        {
        case ModuleId::main: return &distortionTypes;
        case ModuleId::preDistortion: return &distortionTypes;
        case ModuleId::postDistortion: return &distortionTypes;
        case ModuleId::module1: return &noiseTypes;
        case ModuleId::module2: return &preFxTypes;
        case ModuleId::dynamics: return &compTypes;
        case ModuleId::postClip: return &clipTypes;
        case ModuleId::preEmphasis:
        case ModuleId::postEmphasis:
        case ModuleId::count:
        default:
            return nullptr;
        }
    }

    static constexpr int maxParamsPerEffect = 8;


    // can be reused by multiple params for generation., or used for individual parameters. doesnt matter either way.
    struct ParameterInfo
    {
        juce::Identifier id;
        juce::String displayName;
        ValueType unit = ValueType::none;
        juce::NormalisableRange<float> range;
        float defaultValue = 0.0f;
        juce::String paramTooltip;
        int versionHint = 1; // see VERSION HINTS above. never change a released parameter's

        juce::String getParamID() const { return id.toString(); }
        juce::ParameterID getParameterID() const { return { id.toString(), versionHint }; }
    };

    /* global macros, not tied to any slot. these are modulation *sources* for the upcoming mod matrix*/
    static constexpr int numGlobalMacros = 4;

    inline const ParameterInfo globalMacro1{"macro1", "Macro 1", ValueType::percent, makeRange(0.0f, 1.0f), 0.0f,
                                            "Assignable macro knob"};
    inline const ParameterInfo globalMacro2{"macro2", "Macro 2", ValueType::percent, makeRange(0.0f, 1.0f), 0.0f,
                                            "Assignable macro knob"};
    inline const ParameterInfo globalMacro3{"macro3", "Macro 3", ValueType::percent, makeRange(0.0f, 1.0f), 0.0f,
                                            "Assignable macro knob"};
    inline const ParameterInfo globalMacro4{"macro4", "Macro 4", ValueType::percent, makeRange(0.0f, 1.0f), 0.0f,
                                            "Assignable macro knob"};

    inline const ParameterInfo* const globalMacros[numGlobalMacros] {
        &globalMacro1, &globalMacro2, &globalMacro3, &globalMacro4
    };

    static_assert(sizeof(globalMacros) / sizeof(ParameterInfo*) == numGlobalMacros);

    // a macro's name once renamed, kept on the state so it's saved with the preset. unset, it goes by its own
    inline juce::Identifier macroNameProperty (int macro) { return globalMacros[macro]->getParamID() + "Name"; }

    // which macro a parameter is, -1 for the rest
    inline int macroIndexOf (const ParameterInfo& info)
    {
        for (int i = 0; i < numGlobalMacros; ++i)
            if (info.id == globalMacros[i]->id)
                return i;

        return -1;
    }

    // how the main stage arranges its slots, plus the controls the individual routings need
    inline const ParameterInfo mainRouting{"mainRouting", "Routing", ValueType::category, makeSteppedRange(0.0f, choiceSlots - 1.0f), 0.0f,
                                           "How the distortion slots are arranged"};

    // the multiband routing's own setting, kept while another routing is on so coming back finds it as it was
    inline const ParameterInfo bandCount{"bandCount", "Band Count", ValueType::category, makeSteppedRange(2.0f, 4.0f), 2.0f,
                                         "How many bands the multiband routing splits into"};

    inline const ParameterInfo crossoverLow{"crossoverLow", "Crossover Low", ValueType::hz, makeSkewedRange(20.0f, 20000.0f, 0.25f), 200.0f,
                                            "Split point between the first and second band, and the exciter's split point"};
    inline const ParameterInfo crossoverMid{"crossoverMid", "Crossover Mid", ValueType::hz, makeSkewedRange(20.0f, 20000.0f, 0.25f), 1000.0f,
                                            "Split point between the second and third band"};
    inline const ParameterInfo crossoverHigh{"crossoverHigh", "Crossover High", ValueType::hz, makeSkewedRange(20.0f, 20000.0f, 0.25f), 5000.0f,
                                             "Split point between the third and fourth band"};

    inline const ParameterInfo stackCount{"stackCount", "Stack Count", ValueType::category, makeSteppedRange(1.0f, 4.0f), 1.0f,
                                          "The number of distortions applied in a row"};
    inline const ParameterInfo stackFlip{"stackFlip", "Stack Flip Polarity", ValueType::none, makeSteppedRange(0.0f, 1.0f), 1.0f,
                                         "Invert polarity between stack stages, so asymmetric distortion gets evened out"};
    inline const ParameterInfo stackGain{"stackGain", "Stack Stage Gain", ValueType::db, makeRange(-24.0f, 24.0f), -8.0f,
                                         "Increase / decrease gain at each stage"};
    inline const ParameterInfo stackMix{"stackMix", "Stack Dry/Wet", ValueType::percent, makeRange(0.0f, 100.0f), 100.0f,
                                        "Dry / wet of the whole stack, on top of each stage's own dry / wet"};

    inline const ParameterInfo msBalance{"msBalance", "M/S Balance", ValueType::db, makeRange(-24.0f, 24.0f), 0.0f,
                                           "Tilts the balance between mids and sides"};

    // these are duplicated for each slot. do not use directly
    inline const ParameterInfo slotInGain{"inGain", "In Gain", ValueType::db, makeRange(-48.0f, 48.0f), 0.0f,
                                          "Input gain into this module"};
    inline const ParameterInfo slotMix{"dryWet", "Dry/Wet", ValueType::percent, makeRange(0.0f, 100.0f), 100.0f,
                                       "Dry / wet percentage. 100\\% is fully wet"};
    inline const ParameterInfo slotOutGain{"outGain", "Out Gain", ValueType::db, makeRange(-48.0f, 48.0f), 0.0f,
                                           "Makeup gain after this module"};

    // inversely control out gain when in gain is turned up.
    inline const ParameterInfo gainLink{"gainLink", "In/Out Link", ValueType::none, makeSteppedRange(0.0f, 1.0f), 0.0f,
                                        "When enabled, output gain will inversely scale with input."};
    
    static constexpr int numBands = 4;

    inline const ParameterInfo bandMute1{"bandMute1", "Band 1 Mute", ValueType::none, makeSteppedRange(0.0f, 1.0f), 0.0f, "Silence the first band"};
    inline const ParameterInfo bandMute2{"bandMute2", "Band 2 Mute", ValueType::none, makeSteppedRange(0.0f, 1.0f), 0.0f, "Silence the second band"};
    inline const ParameterInfo bandMute3{"bandMute3", "Band 3 Mute", ValueType::none, makeSteppedRange(0.0f, 1.0f), 0.0f, "Silence the third band"};
    inline const ParameterInfo bandMute4{"bandMute4", "Band 4 Mute", ValueType::none, makeSteppedRange(0.0f, 1.0f), 0.0f, "Silence the fourth band"};

    inline const ParameterInfo bandSolo1{"bandSolo1", "Band 1 Solo", ValueType::none, makeSteppedRange(0.0f, 1.0f), 0.0f, "Hear only the soloed bands"};
    inline const ParameterInfo bandSolo2{"bandSolo2", "Band 2 Solo", ValueType::none, makeSteppedRange(0.0f, 1.0f), 0.0f, "Hear only the soloed bands"};
    inline const ParameterInfo bandSolo3{"bandSolo3", "Band 3 Solo", ValueType::none, makeSteppedRange(0.0f, 1.0f), 0.0f, "Hear only the soloed bands"};
    inline const ParameterInfo bandSolo4{"bandSolo4", "Band 4 Solo", ValueType::none, makeSteppedRange(0.0f, 1.0f), 0.0f, "Hear only the soloed bands"};

    inline const ParameterInfo* const bandMutes[numBands] { &bandMute1, &bandMute2, &bandMute3, &bandMute4 };
    inline const ParameterInfo* const bandSolos[numBands] { &bandSolo1, &bandSolo2, &bandSolo3, &bandSolo4 };
    inline const ParameterInfo* const crossoverParams[numBands - 1] { &crossoverLow, &crossoverMid, &crossoverHigh };

    inline const ParameterInfo inputGain{"inputGain", "Input Gain", ValueType::db, makeRange(-24.0f, 24.0f), 0.0f,
                                         "Changes gain of audio entering Hamburger"};
    inline const ParameterInfo outputGain{"outputGain", "Out Gain", ValueType::db, makeRange(-24.0f, 24.0f), 0.0f,
                                          "Changes gain of audio exiting Hamburger"};
    inline const ParameterInfo mix{"mix", "Mix", ValueType::percent, makeRange(0.0f, 100.0f), 100.0f,
                                   "Changes dry/wet mix of the entire plugin"};

    inline const ParameterInfo emphasisLowGain{"emphasisLowGain", "Emphasis Low Gain", ValueType::db, makeRange(-18.0f, 18.0f), 0.0f,
                                               "Boost or cut the lows going into the distortion, applies reverse on the way out"};
    inline const ParameterInfo emphasisHighGain{"emphasisHighGain", "Emphasis Hi Gain", ValueType::db, makeRange(-18.0f, 18.0f), 0.0f,
                                                "Boost or cut the highs going into the distortion, applies reverse on the way out"};
    inline const ParameterInfo emphasisLowFreq{"emphasisLowFreq", "Emphasis Low Frequency", ValueType::hz, makeSkewedRange(20.0f, 20000.0f, 0.25f), 62.0f,
                                               "Frequency of the low emphasis filter"};
    inline const ParameterInfo emphasisHighFreq{"emphasisHighFreq", "Emphasis Hi Frequency", ValueType::hz, makeSkewedRange(20.0f, 20000.0f, 0.25f), 9000.0f,
                                                "Frequency of the high emphasis filter"};

    inline const ParameterInfo emphasisOn{"emphasisOn", "Emphasis EQ On", ValueType::none, makeSteppedRange(0.0f, 1.0f), 1.0f,
                                          "Enables the emphasis EQ around the distortion"};
    inline const ParameterInfo emphasisType{"emphasisType", "Emphasis Type", ValueType::category, makeSteppedRange(0.0f, choiceSlots - 1.0f), 0.0f,
                                            "Two bands of emphasis, or one tilt across the whole spectrum"};
    inline const ParameterInfo emphasisTilt{"emphasisTilt", "Emphasis Tilt", ValueType::db, makeRange(-18.0f, 18.0f), 0.0f,
                                            "Tilts the sound across the whole spectrum after the distortion, and the other way before it: "
                                            "the difference between 20 Hz and 20 kHz"};
    inline const ParameterInfo hamburgerEnabled{"hamburgerEnabled", "Hamburger Enabled", ValueType::none, makeSteppedRange(0.0f, 1.0f), 1.0f,
                                                "Bypasses the entire plugin when off"};

    // the start page. these scale other parameters on their way to the audio without moving their knobs, see scaleFor
    inline const ParameterInfo globalDrive{"globalDrive", "Global Drive", ValueType::percent, makeRange(0.0f, 200.0f), 100.0f,
                                           "Scales every distortion's drive, from none at all up to double where its knob is set. The knobs themselves stay put"};
    inline const ParameterInfo compAmount{"compAmount", "Compressor Amount", ValueType::percent, makeRange(0.0f, 200.0f), 100.0f,
                                          "Scales the compressor's threshold and ratio, or the transient attack and sustain, from nothing up to twice as strong. Speed is left alone"};
    inline const ParameterInfo eqStrength{"eqStrength", "EQ Strength", ValueType::percent, makeRange(-200.0f, 200.0f), 100.0f,
                                          "Scales the emphasis gains, or the tilt. Below 0% flips them"};
    inline const ParameterInfo noiseAmount{"noiseAmount", "Noise Amount", ValueType::percent, makeRange(0.0f, 200.0f), 100.0f,
                                           "Scales how much noise the noise box adds, whichever type it is"};
    inline const ParameterInfo preFxAmount{"preFxAmount", "Pre FX Amount", ValueType::percent, makeRange(0.0f, 200.0f), 100.0f,
                                           "Scales how strong the pre fx box is, whichever type it is"};

    inline const ParameterInfo oversamplingFactor{"oversamplingFactor", "Oversampling Factor", ValueType::oversample, makeSteppedRange(0.0f, 2.0f), 0.0f,
                                                  "Upsample signal internally for reduced aliasing. Very CPU expensive at higher values."};

    // compressor
    inline const ParameterInfo compSpeed{"compSpeed", "Comp Speed", ValueType::ms, makeSkewedRange(0.0f, 400.0f, 0.25f), 100.0f,
                                         "Macro for attack + release parameters. Setting to 0ms will turn compressor into a clipper / saturator, instantly applying gain reduction."};
    inline const ParameterInfo MBCompSpeed{"MBCompSpeed", "Multiband Comp Speed", ValueType::ms, makeSkewedRange(0.0f, 400.0f, 0.25f), 100.0f,
                                           "Macro for attack + release parameters. Setting to 0ms will turn compressor into a clipper / saturator, instantly applying gain reduction."};
    inline const ParameterInfo MSCompSpeed{"MSCompSpeed", "Mid Side Comp Speed", ValueType::ms, makeSkewedRange(0.0f, 400.0f, 0.25f), 100.0f,
                                           "Macro for attack + release parameters. Setting to 0ms will turn compressor into a clipper / saturator, instantly applying gain reduction."};
    inline const ParameterInfo TypeACompSpeed{"TypeACompSpeed", "Type A Comp Speed", ValueType::ms, makeSkewedRange(0.0f, 400.0f, 0.25f), 100.0f,
                                              "Macro for attack + release parameters. Setting to 0ms will turn compressor into a clipper / saturator, instantly applying gain reduction."};

    inline const ParameterInfo compBandTilt{"compBandTilt", "Comp Band Tilt", ValueType::db, makeRange(-20.0f, 20.0f), 0.0f,
                                            "Tilts the threshold across bands, compressing lows or highs harder"};
    inline const ParameterInfo compStereoLink{"compStereoLink", "Stereo Link", ValueType::percent, makeRange(0.0f, 100.0f), 100.0f,
                                              "How much the left and right channels share gain reduction. At 0% each side compresses on its own"};
    inline const ParameterInfo compRatio{"compRatio", "Comp Ratio", ValueType::compressionRatio, makeRange(1.0f, 10.0f), 3.5f,
                                         "How hard the signal is compressed once past the threshold"};

    inline const ParameterInfo stereoCompThreshold{"stereoCompThreshold", "Stereo Comp Threshold", ValueType::db, makeRange(-48.0f, 0.0f), -24.0f,
                                                   "Level at which the stereo compressor starts working"};
    inline const ParameterInfo MBCompThreshold{"MBCompThreshold", "MB Comp Threshold", ValueType::db, makeRange(-48.0f, 0.0f), -24.0f,
                                               "Level at which the multiband compressor starts working"};
    inline const ParameterInfo MSCompThreshold{"MSCompThreshold", "MS Comp Threshold", ValueType::db, makeRange(-48.0f, 0.0f), -24.0f,
                                               "Level at which the mid/side compressor starts working"};

    inline const ParameterInfo TypeAThreshold{"TypeAThreshold", "Type A Threshold", ValueType::db, makeRange(-48.0f, 0.0f), -40.0f,
                                              "Level at which the Type-A compander starts working"};
    inline const ParameterInfo TypeARatio{"TypeARatio", "Type A Ratio", ValueType::compressionRatio, makeRange(1.0f, 4.0f), 2.0f,
                                          "How hard the Type-A compander squashes past the threshold"};
    inline const ParameterInfo TypeATilt{"TypeATilt", "Type A Tilt", ValueType::db, makeRange(-20.0f, 20.0f), -2.0f,
                                         "Tilts the Type-A output gain across bands, favouring lows or highs respectively"};

    inline const ParameterInfo transientAttack{"transientAttack", "Transient Attack", ValueType::db, makeRange(-24.0f, 24.0f), 6.0f,
                                               "Boosts or cuts the start of every hit"};
    inline const ParameterInfo transientSustain{"transientSustain", "Transient Sustain", ValueType::db, makeRange(-24.0f, 24.0f), 0.0f,
                                                "Boosts or cuts what rings out after every hit"};
    inline const ParameterInfo transientSpeed{"transientSpeed", "Transient Speed", ValueType::ms, makeSkewedRange(0.0f, 600.0f, 0.25f), 40.0f,
                                              "How long the start of a hit lasts before the rest of it counts as sustain. At 0ms it shapes every cycle of the waveform instead"};
    inline const ParameterInfo transientLink{"transientLink", "Transient Link", ValueType::percent, makeRange(0.0f, 100.0f), 100.0f,
                                             "How much the left and right channels share their shaping. At 0% each side is shaped on its own"};
    inline const ParameterInfo transientTilt{"transientTilt", "Transient Tilt", ValueType::percent, makeRange(-100.0f, 100.0f), 0.0f,
                                             "Shifts the attack and sustain towards the lows or the highs. At either end one band gets double and the other none"};

    // gate (noise distortion)
    inline const ParameterInfo gateAmt{"gateAmt", "Gate Amt", ValueType::none, makeRange(0.0f, 1.0f), 0.0f,
                                       "The minimum threshold at which gate will let audio pass through"};
    inline const ParameterInfo gateSmooth{"gateSmooth", "Gate Smooth", ValueType::percent, makeRange(0.0f, 100.0f), 0.0f,
                                          "Softens the gate's edge, easing the signal in and out across a knee either side of it instead of snapping"};

    // grunge
    inline const ParameterInfo grungeAmt{"grungeAmt", "Grunge Amt", ValueType::none, makeRange(0.0f, 1.0f), 0.0f,
                                         "Strength of grunge distortion applied"};
    inline const ParameterInfo grungeTone{"grungeTone", "Grunge Tone", ValueType::none, makeRange(0.0f, 1.0f), 0.5f,
                                          "Chooses suitable frequency to resonate at"};

    inline const ParameterInfo subGenAmount{"subGenAmount", "Sub Gen Amount", ValueType::percent, makeRange(0.0f, 100.0f), 50.0f,
                                            "How loud the sine an octave below the lows comes in. At 100% it's as loud as the lows it follows"};
    inline const ParameterInfo hilbertStacks{"hilbertStacks", "Hilbert Stacks", ValueType::none, makeSteppedRange(1.0f, 25.0f), 5.0f,
                                             "How many times over the Hilbert allpasses are run, each smearing the phase further, the lows most"};

    // clipper
    inline const ParameterInfo postClipGain{"postClipGain", "SoftClip Gain", ValueType::db, makeRange(-18.0f, 18.0f), 0.0f,
                                            "The gain of the audio applied into the clipper"};
    inline const ParameterInfo postClipKnee{"postClipKnee", "SoftClip Knee", ValueType::db, makeRange(0.0f, 24.0f), 2.0f,
                                            "How wide the knee is, centred on 0db: the clip starts bending half of it below and holds at 0db half of it above"};
    inline const ParameterInfo postClipTime{"postClipTime", "Limiter Time", ValueType::ms, makeSkewedRange(1.0f, 1000.0f, 0.3f), 50.0f,
                                            "How long the limiter takes to let go after a peak. It sees every peak 1.5ms early and is down by the time it arrives, so nothing gets past 0db"};

    // grill saturation
    inline const ParameterInfo saturationAmount{"saturationAmount", "Grill Saturation", ValueType::percent, makeRange(0.0f, 100.0f), 0.0f,
                                                "Grill Saturation Strength, all-rounder utility distortion with foldback properties at large amplitudes."};
    inline const ParameterInfo diode{"diode", "Grill Diode", ValueType::none, makeRange(0.0f, 100.0f), 0.0f,
                                     "Activates an extremely aggressive clipping"};
    inline const ParameterInfo fold{"fold", "Grill Fold", ValueType::none, makeRange(0.0f, 100.0f), 0.0f,
                                    "Wavefolding, reflects the signal once it goes over 0db"};
    inline const ParameterInfo grillBias{"grillBias", "Grill Bias", ValueType::none, makeRange(0.0f, 1.0f), 0.0f,
                                         "Strength of DC offset before distortion. Adds even harmonics"};
    inline const ParameterInfo grillDcTiming{"grillDcTiming", "Grill DC Env Speed", ValueType::ms, makeSkewedRange(0.0f, 300.0f, 0.25f), 50.0f,
                                             "How fast the DC bias level jumps. Does nothing if DC Bias is 0"};

    // rubidium
    inline const ParameterInfo rubidiumAmount{"rubidiumAmount", "Rubidium Saturation", ValueType::percent, makeRange(0.0f, 100.0f), 5.0f,
                                              "Rubidium Strength, unique distortion that adds crest factor to signal"};
    inline const ParameterInfo rubidiumMojo{"rubidiumMojo", "Rubidium Mojo", ValueType::none, makeRange(0.0f, 100.0f), 5.0f,
                                            "Increases crest factor of signal"};
    inline const ParameterInfo rubidiumAsym{"rubidiumAsym", "Rubidium Asymmetry", ValueType::none, makeRange(0.0f, 10.0f), 0.0f,
                                            "Makes the crest factor shape asymmetrical around zero crossings"};
    inline const ParameterInfo rubidiumTone{"rubidiumTone", "Rubidium Tone", ValueType::hz, makeSkewedRange(4.0f, 100.0f, 0.5f), 5.0f,
                                            "Effectively a high pass, more easily increase crest factor increase to the signal for higher frequencies"};
    inline const ParameterInfo rubidiumBias{"rubidiumBias", "Rubidium Bias", ValueType::none, makeRange(0.0f, 1.0f), 0.0f,
                                            "Applies DC offset before the distortion, creating significant even harmonics"};

    // phase distortion
    inline const ParameterInfo phaseAmount{"phaseAmount", "Phase Distortion", ValueType::percent, makeRange(0.0f, 100.0f), 0.0f,
                                           "Phase Distortion Strength, self modulating signal modulating a tiny delay line"};
    inline const ParameterInfo phaseDistTone{"phaseDistTone", "Phase Dist Tone", ValueType::hz, makeSkewedRange(20.0f, 20000.0f, 0.25f), 355.0f,
                                             "Low pass filter, exclude frequencies above this from self-modulating"};
    inline const ParameterInfo phaseDistStereo{"phaseDistStereo", "Phase Dist Stereo", ValueType::none, makeRange(0.0f, 1.0f), 0.0f,
                                               "Make L and R channels self-modulate independently. By default, both channels are summed before modulating."};
    inline const ParameterInfo phaseRectify{"phaseRectify", "Phase Dist Rectify", ValueType::none, makeRange(0.0f, 1.0f), 0.0f,
                                            "Rectifies the waveform in the modulation path, causing the resulting phase distortion to contain odd harmonics only"};
    inline const ParameterInfo phaseShift{"phaseShift", "Phase Dist Shift", ValueType::none, makeRange(-1.0f, 1.0f), 0.0f,
                                          "Frequency shift the modulation path. Useful at small values for additional movement"};

    // tube distortion
    inline const ParameterInfo tubeAmount{"tubeAmount", "Tube Saturation", ValueType::percent, makeRange(0.0f, 100.0f), 0.0f,
                                          "Tube Saturation Strength, based on an empirical emulation of Class A tube distortion by Will Pirkle"};
    inline const ParameterInfo tubeTone{"tubeTone", "Tube Tone", ValueType::none, makeRange(0.0f, 1.0f), 1.0f,
                                        "Brightness of the tube"};
    inline const ParameterInfo tubeBias{"tubeBias", "Tube Bias", ValueType::none, makeRange(0.0f, 1.0f), 0.0f,
                                        "Offsets the waveform into the tube curve, adding even harmonics"};
    inline const ParameterInfo jeffAmount{"jeffAmount", "Tube Jeff Amt", ValueType::none, makeRange(0.0f, 100.0f), 0.0f,
                                          "Adds an extra aggressive sine waveshaping on top of the signal"};

    // tape hysteresis
    inline const ParameterInfo tapeBias{"tapeBias", "Tape Bias", ValueType::none, makeRange(0.0f, 1.0f), 0.0f,
                                        "Applies DC biasing to the signal before distortion, adding even harmonics"};
    inline const ParameterInfo tapeDrive{"tapeDrive", "Tape Drive", ValueType::none, makeRange(0.0f, 1.0f), 0.0f,
                                         "Tape Saturation Strength, based on a realistic magnetic tape hysteresis model"};
    inline const ParameterInfo tapeWidth{"tapeWidth", "Tape Age", ValueType::none, makeRange(0.0f, 1.0f), 0.3f,
                                         "Ages the tape, making low end subtly more pronounced by widening the hysteresis curve"};

    // sizzle
    inline const ParameterInfo sizzleAmount{"sizzleAmount", "Sizzle Amt", ValueType::none, makeRange(0.0f, 100.0f), 5.0f,
                                            "Sizzle Noise strength, adds noise in zero crossings of the signal."};
    inline const ParameterInfo sizzleFrequency{"sizzleFrequency", "Sizzle Freq", ValueType::none, makeSkewedRange(20.0f, 20000.0f, 0.25f), 4000.0f,
                                               "Frequency the sizzle noise sits around"};
    inline const ParameterInfo sizzleQ{"sizzleQ", "Sizzle Q", ValueType::none, makeRange(0.1f, 1.5f), 1.0f,
                                       "Size of sizzle's amplitude band width around the zero crossing"};

    inline const ParameterInfo fizzAmount{"fizzAmount", "Fizz Amt", ValueType::none, makeRange(0.0f, 100.0f), 5.0f,
                                          "Amount of noise added to the signal. Noise folds back past halfway"};

    // erosion
    inline const ParameterInfo erosionAmount{"erosionAmount", "Erosion Amt", ValueType::none, makeRange(0.0f, 100.0f), 3.0f,
                                             "Erosion noise strength. Applies noise similar to Ableton's erosion device"};
    inline const ParameterInfo erosionFrequency{"erosionFrequency", "Noise Freq", ValueType::hz, makeSkewedRange(20.0f, 20000.0f, 0.25f), 400.0f,
                                                "Frequency the erosion noise resonates at."};
    inline const ParameterInfo erosionQ{"erosionQ", "Erosion Q", ValueType::none, makeRange(0.1f, 1.5f), 1.0f,
                                        "Erosion Filter Q factor, resonance amount"};

    inline const ParameterInfo downsampleFreq{"downsampleFreq", "Dwnsmpl Freq", ValueType::hz, makeSkewedRange(200.0f, 40000.0f, 0.25f), 40000.0f,
                                              "The new sample rate the signal is crushed to"};
    inline const ParameterInfo bitReduction{"bitReduction", "Dwnsmpl Bits", ValueType::none, makeRange(1.0f, 32.0f), 32.0f,
                                            "Bit depth the signal is quantised to"};

    // allpass
    inline const ParameterInfo allPassFreq{"allPassFreq", "AllPass Frequency", ValueType::hz, makeSkewedRange(20.0f, 20000.0f, 0.25f), 85.0f,
                                           "The frequency the allpass filters center around"};
    inline const ParameterInfo allPassQ{"allPassQ", "AllPass Q", ValueType::none, makeRange(0.01f, 1.41f), 0.4f,
                                        "Resonance of the allpass filters"};
    inline const ParameterInfo allPassAmount{"allPassAmount", "AllPass Number", ValueType::none, makeRange(0.0f, 50.0f), 10.0f,
                                             "# of Allpasses in the stack, higher number results in transient smearing"};

    // slew
    inline const ParameterInfo alphaParam{"alphaParam", "Strength", ValueType::none, makeRange(0.0f, 1.0f), 1.0f,
                                          "Slew distortion strength, a model that modifies \"rate of change\" of signal. If you can't hear changes, set this to max."};
    inline const ParameterInfo slewSpeed{"slewSpeed", "Slew Tone", ValueType::none, makeRange(0.0f, 1.0f), 0.5f,
                                         "Slew speed, controls maximum speed that signal can track onto, acting as a fake lowpass control"};
    inline const ParameterInfo directionality{"directionality", "Slew Bias", ValueType::none, makeRange(-1.0f, 1.0f), 0.0f,
                                              "Bias the slew limiting towards rising or falling edges"};
    inline const ParameterInfo slewType{"slewType", "Slew Type", ValueType::category, makeSteppedRange(0.0f, 2.0f), 0.0f,
                                        "Select a slew limiting algorithm to use. Non-standard slew algorithms are present after the first one."};
                                              
    // waveshape
    inline const ParameterInfo waveshapeDrive{"waveshapeDrive", "Waveshape Drive", ValueType::db, makeRange(-24.0f, 24.0f), 0.0f,
                                              "Gain into the waveshaper, pushing the signal further along the curve"};
    inline const ParameterInfo waveshapeX{"waveshapeX", "Waveshape X", ValueType::none, makeRange(0.0f, 1.0f), 0.5f,
                                          "Across the waveshape map, where shapes with similar harmonics sit together. The pad blends the nearest few"};
    inline const ParameterInfo waveshapeY{"waveshapeY", "Waveshape Y", ValueType::none, makeRange(0.0f, 1.0f), 0.5f,
                                          "Up the waveshape map, where shapes with similar harmonics sit together. The pad blends the nearest few"};
    inline const ParameterInfo waveshapeSmooth{"waveshapeSmooth", "Waveshape Smooth", ValueType::percent, makeRange(0.0f, 100.0f), 0.0f,
                                               "Rounds off sharp corners in the curve by averaging it over a wider area, without filtering the audio"};
    inline const ParameterInfo waveshapeBias{"waveshapeBias", "Waveshape Bias", ValueType::none, makeRange(-1.0f, 1.0f), 0.0f,
                                             "Offsets the signal into the curve for even harmonics. The offset's own output is taken back out, so silence stays silent"};
    inline const ParameterInfo waveshapeAsym{"waveshapeAsym", "Waveshape Asym", ValueType::percent, makeRange(0.0f, 100.0f), 0.0f,
                                             "Skews the signal before the curve: the negative side flattens out while the positive side accelerates up"};


    // opto: audio clipped by LEDs and turned down by the LDR they light as a distortion, or only lighting them as a compressor
    inline const ParameterInfo optoDrive{"optoDrive", "Opto Drive", ValueType::db, makeRange(0.0f, 24.0f), 6.0f,
                                         "How hard the audio is driven into the LEDs, clipping past their forward voltage and lighting the cell that turns it back down"};
    inline const ParameterInfo optoThreshold{"optoThreshold", "Opto Threshold", ValueType::db, makeRange(-48.0f, 0.0f), -18.0f,
                                             "Where the LEDs start to light the cell, and so where the audio starts being turned down"};
    inline const ParameterInfo optoRatio{"optoRatio", "Opto Ratio", ValueType::compressionRatio, makeRange(1.0f, 20.0f), 4.0f,
                                         "How hard the cell turns the audio down once it's lit past the threshold"};
    inline const ParameterInfo optoCompSpeed{"optoCompSpeed", "Opto Speed", ValueType::ms, makeSkewedRange(0.0f, 100.0f, 0.4f), 5.0f,
                                             "How quickly the cell lights up and lets go, letting go ten and a half times slower than it lights, as an LA-2A's does"};
    inline const ParameterInfo optoBias{"optoBias", "Opto DC Bias", ValueType::none, makeRange(0.0f, 1.0f), 0.0f,
                                        "Pushes the LEDs further off their resting point the louder it gets, like grill's bias"};
    inline const ParameterInfo optoDcSpeed{"optoDcSpeed", "Opto DC Speed", ValueType::ms, makeSkewedRange(0.0f, 300.0f, 0.25f), 50.0f,
                                           "How quickly the bias follows the level"};
    inline const ParameterInfo optoDamping{"optoDamping", "Opto Damping", ValueType::none, makeSkewedRange(0.1f, 4.0f, 0.5f), 1.0f,
                                           "How the opto distortion's cell and bias settle. 1 is critically damped, eased in with no overshoot. Lower overshoots and rings back, higher creeps in"};


    // dll: debug builds only, whatever the loaded dylib makes of them, see hotdsp/HotDsp.h
    inline const ParameterInfo dllDrive{"dllDrive", "DLL Drive", ValueType::db, makeRange(-24.0f, 24.0f), 0.0f, "DLL drive"};
    inline const ParameterInfo dllA{"dllA", "DLL A", ValueType::none, makeRange(0.0f, 1.0f), 0.5f, "DLL parameter A"};
    inline const ParameterInfo dllB{"dllB", "DLL B", ValueType::none, makeRange(0.0f, 1.0f), 0.5f, "DLL parameter B"};
    inline const ParameterInfo dllC{"dllC", "DLL C", ValueType::none, makeRange(0.0f, 1.0f), 0.5f, "DLL parameter C"};
    inline const ParameterInfo dllD{"dllD", "DLL D", ValueType::none, makeRange(0.0f, 1.0f), 0.5f, "DLL parameter D"};
    inline const ParameterInfo dllE{"dllE", "DLL E", ValueType::none, makeRange(0.0f, 1.0f), 0.5f, "DLL parameter E"};
    inline const ParameterInfo dllF{"dllF", "DLL F", ValueType::none, makeRange(0.0f, 1.0f), 0.5f, "DLL parameter F"};
}

inline juce::ParameterID paramIdFor (SlotId slot, const ParamIDs::ParameterInfo& descriptor)
{
    return { slot.prefix() + "_" + descriptor.id.toString(), descriptor.versionHint };
}
