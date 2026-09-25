#pragma once

#include "juce_core/juce_core.h"
#include "juce_audio_processors/juce_audio_processors.h"
#include "KnobUtils.h"

inline auto makeRange(float start, float end)
{
    return juce::NormalisableRange<float>(start, end, 0.001f);
}

// (min, max, interval, skew) — as used by the frequency-style knobs
inline auto makeSkewedRange(float start, float end, float skew)
{
    return juce::NormalisableRange<float>(start, end, 0.0f, skew);
}

// stepped range for choice / bool / int parameters
inline auto makeSteppedRange(float start, float end)
{
    return juce::NormalisableRange<float>(start, end, 1.0f);
}

// all the module IDs we will ever need
enum class ModuleId : uint8_t
{
    preEmphasis,
    dynamics,
    module1,
    module2,
    preDistortion,
    main,
    postDistortion,
    postEmphasis,
    postClip,
    count
};

using RoutingOrder = std::array<ModuleId, static_cast<size_t> (ModuleId::count)>;

// 16 items fit in an atomic if each item is represented as a uint4 so we can just cheat threading
static constexpr size_t routingBitsPerSlot = 4;

static_assert (static_cast<size_t> (ModuleId::count) <= 64 / routingBitsPerSlot);

inline constexpr juce::uint64 packRouting (const RoutingOrder& order)
{
    juce::uint64 packed = 0;

    for (size_t i = 0; i < order.size(); ++i)
        packed |= static_cast<juce::uint64> (order[i]) << (i * routingBitsPerSlot);

    return packed;
}

inline constexpr RoutingOrder unpackRouting (juce::uint64 packed)
{
    RoutingOrder order {};

    for (size_t i = 0; i < order.size(); ++i)
        order[i] = static_cast<ModuleId> ((packed >> (i * routingBitsPerSlot)) & 0xf);

    return order;
}

struct ModuleInfo
{
    ModuleId id;
    const char *key;
    const char *displayName;
    int slotCount = 1; // set to 0 to ignore generating macros, and hand roll the params yourself. set to 1 or higher for macros + slolts
    bool enabledByDefault = true; // initial state of the slot's generated on/off toggle
    int versionHint = 1; // for the module's type and enabled parameters, see VERSION HINTS in ParamIDs
};

static constexpr ModuleInfo pluginModules[]{
    {ModuleId::preEmphasis, "preEmphasis", "PRE-EMPH", 0},
    {ModuleId::dynamics, "dynamics", "COMP", 1, false},
    {ModuleId::module1, "module1", "FX1", 1, false},
    {ModuleId::module2, "module2", "FX2", 1, false},
    {ModuleId::preDistortion, "preDistortion", "PRE", 1, false},
    {ModuleId::main, "main", "MAIN", 4, true},
    {ModuleId::postDistortion, "postDistortion", "POST", 1, false},
    {ModuleId::postEmphasis, "postEmphasis", "POST-EMPH", 0},
    {ModuleId::postClip, "postClip", "CLIP", 1, true},
};

// if we are missing a module ID this will catch it
static_assert (std::size (pluginModules) == static_cast<size_t> (ModuleId::count));

// on compile time, generate the default routing we wanna use
static constexpr RoutingOrder defaultRouting = [](){
    RoutingOrder chain {};

    for (size_t i = 0; i < std::size (pluginModules); ++i)
        chain[i] = pluginModules[i].id;

    return chain;
}();

// a slot is one instance of a module, owning its own copy of that module's parameters
// so a slot id is what scopes a descriptor to a concrete parameter
struct SlotId
{
    ModuleId module = ModuleId::main;
    int sub = 0;

    juce::String prefix() const
    {
        auto p = juce::String (pluginModules[(size_t) module].key);
        return sub == 0 ? p : p + "_slot" + juce::String (sub);
    }

    juce::String displayNamePrefix() const
    {
        const auto& m = pluginModules[(size_t) module];
        auto p = juce::String (m.displayName);

        // numbered from 1, and only when the module actually has more than one slot
        return m.slotCount > 1 ? p + " " + juce::String (sub + 1) : p;
    }

    int versionHint() const { return pluginModules[(size_t) module].versionHint; }

    juce::ParameterID type()    const { return { prefix() + "_type",    versionHint() }; }
    juce::ParameterID enabled() const { return { prefix() + "_enabled", versionHint() }; }
};

namespace ParamIDs
{
    // the selectable algorithm names a module's type parameter offers, in type index order
    struct PanelInfo
    {
        juce::StringArray categories;
    };

    /*  VERSION HINTS. Audio Units hand hosts the parameters sorted by version hint, then by ID, and Logic and GarageBand
        keep automation by that order: a parameter that sorts in among the released ones moves their automation onto
        the wrong parameters. So every parameter added after a release needs a higher hint than anything released, set
        on its ParameterInfo (or ModuleInfo, for a whole new module).

        When a version ships, set releasedVersionHint to the highest hint in it and releasedParameterCount to its total
        parameter count, printed in a debug build. After that, new parameters take releasedVersionHint + 1, and a debug
        build asserts on start up if a parameter at a released hint was added, or one removed. */
    static constexpr int releasedVersionHint = 1;
    static constexpr int releasedParameterCount = 0; // 0 until v0.10 ships, which leaves the check off

    struct ParameterInfo
    {
        juce::Identifier id;
        juce::String displayName;
        ParamUnits unit = ParamUnits::none;
        juce::NormalisableRange<float> range;
        float defaultValue = 0.0f;
        juce::String paramTooltip;
        int versionHint = 1; // see VERSION HINTS above. never change a released parameter's

        juce::String getParamID() const { return id.toString(); }
        juce::ParameterID getParameterID() const { return { id.toString(), versionHint }; }
    };

    static constexpr int choiceSlots = 16;

    inline juce::StringArray withReservedSlots (juce::StringArray names)
    {
        jassert (names.size() <= choiceSlots);

        for (int i = names.size(); i < choiceSlots; ++i)
            names.add ("RESERVED " + juce::String (i + 1));

        return names;
    }

    /*  The names every type goes by on screen: in its box's type menu, which reads them from here, and in the FX order.
        Only renamed or added to the end, never reordered or removed: a preset stores the index. */
    static const PanelInfo distortionTypes { juce::StringArray({"GRILL", "TUBE", "PHASE", "RUBIDIUM", "TAPE", "SLEW", "WAVESHAPE"}) };
    static const PanelInfo compTypes { juce::StringArray({"STEREO", "MB", "MS", "TYPE A"}) };
    static const PanelInfo noiseTypes { juce::StringArray({"SIZZLE", "EROSION", "BIT", "GATE", "FIZZ"}) };
    static const PanelInfo preFxTypes { juce::StringArray({"ALLPASS", "GRUNGE"}) };

    // keep in step with MainRouting::Routing
    static const PanelInfo routingTypes { juce::StringArray({"STACK", "MULTIBAND", "MID/SIDE", "EXCITER"}) };

    // mapping between module IDs and the categories that each module contains. used for auto param generation
    inline const PanelInfo* categoriesFor(ModuleId id)
    {
        switch (id)
        {
        case ModuleId::main: return &distortionTypes;
        case ModuleId::preDistortion: return &distortionTypes;
        case ModuleId::postDistortion: return &distortionTypes;
        case ModuleId::module1: return &noiseTypes;
        case ModuleId::module2: return &preFxTypes;
        case ModuleId::dynamics: return &compTypes;
        case ModuleId::preEmphasis:
        case ModuleId::postEmphasis:
        case ModuleId::postClip:
        case ModuleId::count:
        default:
            return nullptr;
        }
    }

    static constexpr int maxParamsPerEffect = 8;

    // one effect's macro layout; which module and type name it belongs to lives in pluginModules
    struct EffectInfo
    {
        std::array<ParameterInfo, maxParamsPerEffect> params;
    };

    /* global macros, not tied to any slot. these are modulation *sources* for the upcoming mod matrix*/
    static constexpr int numGlobalMacros = 4;

    static const ParameterInfo globalMacro1{"macro1", "Macro 1", ParamUnits::percent, makeRange(0.0f, 1.0f), 0.0f,
                                            "Assignable macro knob"};
    static const ParameterInfo globalMacro2{"macro2", "Macro 2", ParamUnits::percent, makeRange(0.0f, 1.0f), 0.0f,
                                            "Assignable macro knob"};
    static const ParameterInfo globalMacro3{"macro3", "Macro 3", ParamUnits::percent, makeRange(0.0f, 1.0f), 0.0f,
                                            "Assignable macro knob"};
    static const ParameterInfo globalMacro4{"macro4", "Macro 4", ParamUnits::percent, makeRange(0.0f, 1.0f), 0.0f,
                                            "Assignable macro knob"};

    static const ParameterInfo* const globalMacros[numGlobalMacros] {
        &globalMacro1, &globalMacro2, &globalMacro3, &globalMacro4
    };

    static_assert(sizeof(globalMacros) / sizeof(ParameterInfo*) == numGlobalMacros);

    // how the main stage arranges its slots, plus the controls the individual routings need
    static const ParameterInfo mainRouting{"mainRouting", "Routing", ParamUnits::category, makeSteppedRange(0.0f, choiceSlots - 1.0f), 0.0f,
                                           "How the distortion slots are arranged"};

    // the multiband routing's own setting, kept while another routing is on so coming back finds it as it was
    static const ParameterInfo bandCount{"bandCount", "Band Count", ParamUnits::category, makeSteppedRange(2.0f, 4.0f), 2.0f,
                                         "How many bands the multiband routing splits into"};

    static const ParameterInfo crossoverLow{"crossoverLow", "Crossover Low", ParamUnits::hz, makeSkewedRange(20.0f, 20000.0f, 0.25f), 200.0f,
                                            "Split point between the first and second band, and the exciter's split point"};
    static const ParameterInfo crossoverMid{"crossoverMid", "Crossover Mid", ParamUnits::hz, makeSkewedRange(20.0f, 20000.0f, 0.25f), 1000.0f,
                                            "Split point between the second and third band"};
    static const ParameterInfo crossoverHigh{"crossoverHigh", "Crossover High", ParamUnits::hz, makeSkewedRange(20.0f, 20000.0f, 0.25f), 5000.0f,
                                             "Split point between the third and fourth band"};

    static const ParameterInfo stackCount{"stackCount", "Stack Count", ParamUnits::category, makeSteppedRange(1.0f, 4.0f), 1.0f,
                                          "The number of distortions applied in a row"};
    static const ParameterInfo stackFlip{"stackFlip", "Stack Flip Polarity", ParamUnits::none, makeSteppedRange(0.0f, 1.0f), 1.0f,
                                         "Invert polarity between stack stages, so asymmetric distortion gets evened out"};
    static const ParameterInfo stackGain{"stackGain", "Stack Stage Gain", ParamUnits::db, makeRange(-24.0f, 24.0f), -8.0f,
                                         "Increase / decrease gain at each stage"};
    static const ParameterInfo stackMix{"stackMix", "Stack Dry/Wet", ParamUnits::percent, makeRange(0.0f, 100.0f), 100.0f,
                                        "Dry / wet of the whole stack, on top of each stage's own dry / wet"};

    //  keep in line with MainRouting::StackFilter
    static const PanelInfo stackFilterTypes { juce::StringArray({"NO FILTER", "ALLPASS", "HILBERT", "BANDPASS", "NOTCH"}) };
    static const ParameterInfo stackFilter{"stackFilter", "Stack Filter", ParamUnits::category, makeSteppedRange(0.0f, choiceSlots - 1.0f), 0.0f,
                                           "Choose a filter between stack stages"};
    static const ParameterInfo stackFilterFreq{"stackFilterFreq", "Stack Filter Freq", ParamUnits::hz, makeSkewedRange(20.0f, 20000.0f, 0.25f), 1000.0f,
                                               "Cutoff / center frequency of filter effect"};
    static const ParameterInfo stackFilterQ{"stackFilterQ", "Stack Filter Q", ParamUnits::none, makeSkewedRange(0.1f, 10.0f, 0.3f), 0.707f,
                                            "How narrow the bandpass or notch between stages is"};
    static const ParameterInfo stackRotation{"stackRotation", "Stack Rotation", ParamUnits::degrees, makeRange(0.0f, 90.0f), 45.0f,
                                             "How far to rotate phase of all frequencies every stage"};

    static const ParameterInfo msBalance{"msBalance", "M/S Balance", ParamUnits::db, makeRange(-24.0f, 24.0f), 0.0f,
                                           "Tilts the balance between mids and sides"};

    // these are duplicated for each slot. do not use directly
    static const ParameterInfo slotInGain{"inGain", "In Gain", ParamUnits::db, makeRange(-48.0f, 48.0f), 0.0f,
                                          "Input gain into this distortion"};
    static const ParameterInfo slotMix{"dryWet", "Dry/Wet", ParamUnits::percent, makeRange(0.0f, 100.0f), 100.0f,
                                       "Dry / wet percentage. 100\\% is fully wet"};
    static const ParameterInfo slotOutGain{"outGain", "Out Gain", ParamUnits::db, makeRange(-48.0f, 48.0f), 0.0f,
                                           "Makeup gain after distorting"};

    // inversely control out gain when in gain is turned up.
    static const ParameterInfo gainLink{"gainLink", "In/Out Link", ParamUnits::none, makeSteppedRange(0.0f, 1.0f), 0.0f,
                                        "When enabled, output gain will inversely scale with input."};

    
    static constexpr int numBands = 4;

    static const ParameterInfo bandMute1{"bandMute1", "Band 1 Mute", ParamUnits::none, makeSteppedRange(0.0f, 1.0f), 0.0f, "Silence the first band"};
    static const ParameterInfo bandMute2{"bandMute2", "Band 2 Mute", ParamUnits::none, makeSteppedRange(0.0f, 1.0f), 0.0f, "Silence the second band"};
    static const ParameterInfo bandMute3{"bandMute3", "Band 3 Mute", ParamUnits::none, makeSteppedRange(0.0f, 1.0f), 0.0f, "Silence the third band"};
    static const ParameterInfo bandMute4{"bandMute4", "Band 4 Mute", ParamUnits::none, makeSteppedRange(0.0f, 1.0f), 0.0f, "Silence the fourth band"};

    static const ParameterInfo bandSolo1{"bandSolo1", "Band 1 Solo", ParamUnits::none, makeSteppedRange(0.0f, 1.0f), 0.0f, "Hear only the soloed bands"};
    static const ParameterInfo bandSolo2{"bandSolo2", "Band 2 Solo", ParamUnits::none, makeSteppedRange(0.0f, 1.0f), 0.0f, "Hear only the soloed bands"};
    static const ParameterInfo bandSolo3{"bandSolo3", "Band 3 Solo", ParamUnits::none, makeSteppedRange(0.0f, 1.0f), 0.0f, "Hear only the soloed bands"};
    static const ParameterInfo bandSolo4{"bandSolo4", "Band 4 Solo", ParamUnits::none, makeSteppedRange(0.0f, 1.0f), 0.0f, "Hear only the soloed bands"};

    static const ParameterInfo* const bandMutes[numBands] { &bandMute1, &bandMute2, &bandMute3, &bandMute4 };
    static const ParameterInfo* const bandSolos[numBands] { &bandSolo1, &bandSolo2, &bandSolo3, &bandSolo4 };
    static const ParameterInfo* const crossoverParams[numBands - 1] { &crossoverLow, &crossoverMid, &crossoverHigh };

    static const ParameterInfo inputGain{"inputGain", "Input Gain", ParamUnits::db, makeRange(-24.0f, 24.0f), 0.0f,
                                         "Changes gain of audio entering Hamburger"};
    static const ParameterInfo outputGain{"outputGain", "Out Gain", ParamUnits::db, makeRange(-24.0f, 24.0f), 0.0f,
                                          "Changes gain of audio exiting Hamburger"};
    static const ParameterInfo mix{"mix", "Mix", ParamUnits::percent, makeRange(0.0f, 100.0f), 100.0f,
                                   "Changes dry/wet mix of the entire plugin"};

    static const ParameterInfo emphasisLowGain{"emphasisLowGain", "Emphasis Low Gain", ParamUnits::db, makeRange(-18.0f, 18.0f), 0.0f,
                                               "Boost or cut the lows going into the distortion, applies reverse on the way out"};
    static const ParameterInfo emphasisHighGain{"emphasisHighGain", "Emphasis Hi Gain", ParamUnits::db, makeRange(-18.0f, 18.0f), 0.0f,
                                                "Boost or cut the highs going into the distortion, applies reverse on the way out"};
    static const ParameterInfo emphasisLowFreq{"emphasisLowFreq", "Emphasis Low Frequency", ParamUnits::hz, makeSkewedRange(20.0f, 20000.0f, 0.25f), 62.0f,
                                               "Frequency of the low emphasis filter"};
    static const ParameterInfo emphasisHighFreq{"emphasisHighFreq", "Emphasis Hi Frequency", ParamUnits::hz, makeSkewedRange(20.0f, 20000.0f, 0.25f), 9000.0f,
                                                "Frequency of the high emphasis filter"};

    static const ParameterInfo emphasisOn{"emphasisOn", "Emphasis EQ On", ParamUnits::none, makeSteppedRange(0.0f, 1.0f), 1.0f,
                                          "Enables the emphasis EQ around the distortion"};
    static const ParameterInfo hamburgerEnabled{"hamburgerEnabled", "Hamburger Enabled", ParamUnits::none, makeSteppedRange(0.0f, 1.0f), 1.0f,
                                                "Bypasses the entire plugin when off"};

    static const ParameterInfo oversamplingFactor{"oversamplingFactor", "Oversampling Factor", ParamUnits::oversample, makeSteppedRange(0.0f, 2.0f), 0.0f,
                                                  "Upsample signal internally for reduced aliasing. Very CPU expensive at higher values."};

    // compressor
    static const ParameterInfo compSpeed{"compSpeed", "Comp Speed", ParamUnits::ms, makeSkewedRange(0.0f, 400.0f, 0.25f), 100.0f,
                                         "Macro for attack + release parameters. Setting to 0ms will turn compressor into a clipper / saturator, instantly applying gain reduction."};
    static const ParameterInfo MBCompSpeed{"MBCompSpeed", "Multiband Comp Speed", ParamUnits::ms, makeSkewedRange(0.0f, 400.0f, 0.25f), 100.0f,
                                           "Macro for attack + release parameters. Setting to 0ms will turn compressor into a clipper / saturator, instantly applying gain reduction."};
    static const ParameterInfo MSCompSpeed{"MSCompSpeed", "Mid Side Comp Speed", ParamUnits::ms, makeSkewedRange(0.0f, 400.0f, 0.25f), 100.0f,
                                           "Macro for attack + release parameters. Setting to 0ms will turn compressor into a clipper / saturator, instantly applying gain reduction."};
    static const ParameterInfo TypeACompSpeed{"TypeACompSpeed", "Type A Comp Speed", ParamUnits::ms, makeSkewedRange(0.0f, 400.0f, 0.25f), 100.0f,
                                              "Macro for attack + release parameters. Setting to 0ms will turn compressor into a clipper / saturator, instantly applying gain reduction."};

    static const ParameterInfo compBandTilt{"compBandTilt", "Comp Band Tilt", ParamUnits::db, makeRange(-20.0f, 20.0f), 0.0f,
                                            "Tilts the threshold across bands, compressing lows or highs harder"};
    static const ParameterInfo compStereoLink{"compStereoLink", "Stereo Link", ParamUnits::percent, makeRange(0.0f, 100.0f), 100.0f,
                                              "How much the left and right channels share gain reduction"};
    static const ParameterInfo compRatio{"compRatio", "Comp Ratio", ParamUnits::compressionRatio, makeRange(1.0f, 10.0f), 3.5f,
                                         "How hard the signal is compressed once past the threshold"};
    static const ParameterInfo compOut{"compOut", "Comp Makeup", ParamUnits::db, makeRange(-24.0f, 24.0f), 0.0f,
                                       "Makeup gain applied after compression"};

    static const ParameterInfo stereoCompThreshold{"stereoCompThreshold", "Stereo Comp Threshold", ParamUnits::db, makeRange(-48.0f, 0.0f), -24.0f,
                                                   "Level at which the stereo compressor starts working"};
    static const ParameterInfo MBCompThreshold{"MBCompThreshold", "MB Comp Threshold", ParamUnits::db, makeRange(-48.0f, 0.0f), -24.0f,
                                               "Level at which the multiband compressor starts working"};
    static const ParameterInfo MSCompThreshold{"MSCompThreshold", "MS Comp Threshold", ParamUnits::db, makeRange(-48.0f, 0.0f), -24.0f,
                                               "Level at which the mid/side compressor starts working"};

    static const ParameterInfo TypeAThreshold{"TypeAThreshold", "Type A Threshold", ParamUnits::db, makeRange(-48.0f, 0.0f), -40.0f,
                                              "Level at which the Type-A compander starts working"};
    static const ParameterInfo TypeARatio{"TypeARatio", "Type A Ratio", ParamUnits::compressionRatio, makeRange(1.0f, 4.0f), 2.0f,
                                          "How hard the Type-A compander squashes past the threshold"};
    static const ParameterInfo TypeATilt{"TypeATilt", "Type A Tilt", ParamUnits::db, makeRange(-20.0f, 20.0f), -2.0f,
                                         "Tilts the Type-A output gain across bands, favouring lows or highs respectively"};
    static const ParameterInfo TypeAOut{"TypeAOut", "Type A Out", ParamUnits::db, makeRange(-24.0f, 24.0f), -12.0f,
                                        "Output gain applied after the Type-A compander"};

    // gate (noise distortion)
    static const ParameterInfo gateAmt{"gateAmt", "Gate Amt", ParamUnits::none, makeRange(0.0f, 1.0f), 0.0f,
                                       "The minimum threshold at which gate will let audio pass through"};
    static const ParameterInfo gateMix{"gateMix", "Gate Mix", ParamUnits::none, makeRange(0.0f, 1.0f), 1.0f,
                                       "Blends the gated signal with the dry signal"};

    // grunge
    static const ParameterInfo grungeAmt{"grungeAmt", "Grunge Amt", ParamUnits::none, makeRange(0.0f, 1.0f), 0.0f,
                                         "Strength of grunge distortion applied"};
    static const ParameterInfo grungeTone{"grungeTone", "Grunge Tone", ParamUnits::none, makeRange(0.0f, 1.0f), 0.5f,
                                          "Chooses suitable frequency to resonate at"};

    // clipper
    static const ParameterInfo postClipGain{"postClipGain", "SoftClip Gain", ParamUnits::db, makeRange(-18.0f, 18.0f), 0.0f,
                                            "The gain of the audio applied into the clipper"};
    static const ParameterInfo postClipKnee{"postClipKnee", "SoftClip Knee", ParamUnits::db, makeRange(0.0f, 4.0f), 0.5f,
                                            "The soft knee width of the clipper before it hits 0db"};

    // grill saturation
    static const ParameterInfo saturationAmount{"saturationAmount", "Grill Saturation", ParamUnits::percent, makeRange(0.0f, 100.0f), 0.0f,
                                                "Grill Saturation Strength, all-rounder utility distortion with foldback properties at large amplitudes."};
    static const ParameterInfo diode{"diode", "Grill Diode", ParamUnits::none, makeRange(0.0f, 100.0f), 0.0f,
                                     "Activates an extremely aggressive clipping"};
    static const ParameterInfo fold{"fold", "Grill Fold", ParamUnits::none, makeRange(0.0f, 100.0f), 0.0f,
                                    "Wavefolding, reflects the signal once it goes over 0db"};
    static const ParameterInfo grillBias{"grillBias", "Grill Bias", ParamUnits::none, makeRange(0.0f, 1.0f), 0.0f,
                                         "Strength of DC offset before distortion. Adds even harmonics"};
    static const ParameterInfo grillDcTiming{"grillDcTiming", "Grill DC Env Speed", ParamUnits::ms, makeSkewedRange(0.0f, 300.0f, 0.25f), 50.0f,
                                             "How fast the DC bias level jumps. Does nothing if DC Bias is 0"};

    // matrix distortion
    static const ParameterInfo matrix1{"matrix1", "Matrix #1", ParamUnits::none, makeRange(0.0f, 1.0f), 0.0f,
                                       "Drive into the matrix waveshaper"};
    static const ParameterInfo matrix2{"matrix2", "Matrix #2", ParamUnits::none, makeRange(0.0f, 1.0f), 0.0f,
                                       "Add gritty ripple to the wave"};
    static const ParameterInfo matrix3{"matrix3", "Matrix #3", ParamUnits::none, makeRange(0.0f, 1.0f), 0.0f,
                                       "Bounce / Wavefold"};
    static const ParameterInfo matrix4{"matrix4", "Matrix #4", ParamUnits::none, makeRange(0.0f, 1.0f), 0.0f,
                                       "Crushing / digital stepping"};
    static const ParameterInfo matrix5{"matrix5", "Matrix #5", ParamUnits::none, makeRange(0.0f, 1.0f), 0.0f,
                                       "Morphs between the filter shapes feeding the waveshaper"};
    static const ParameterInfo matrix6{"matrix6", "Matrix #6", ParamUnits::none, makeRange(0.0f, 1.0f), 0.0f,
                                       "Cutoff frequency of the matrix filters"};
    static const ParameterInfo matrix7{"matrix7", "Matrix #7", ParamUnits::none, makeRange(0.0f, 1.0f), 0.0f,
                                       "Currently unused"};
    static const ParameterInfo matrix8{"matrix8", "Matrix #8", ParamUnits::none, makeRange(0.0f, 1.0f), 0.0f,
                                       "Currently unused"};
    static const ParameterInfo matrix9{"matrix9", "Matrix #9", ParamUnits::none, makeRange(0.0f, 1.0f), 1.0f,
                                       "Blends the filtered signal against the dry one before shaping"};

    // rubidium
    static const ParameterInfo rubidiumAmount{"rubidiumAmount", "Rubidium Saturation", ParamUnits::percent, makeRange(0.0f, 100.0f), 5.0f,
                                              "Rubidium Strength, unique distortion that adds crest factor to signal"};
    static const ParameterInfo rubidiumMojo{"rubidiumMojo", "Rubidium Mojo", ParamUnits::none, makeRange(0.0f, 100.0f), 5.0f,
                                            "Increases crest factor of signal"};
    static const ParameterInfo rubidiumAsym{"rubidiumAsym", "Rubidium Asymmetry", ParamUnits::none, makeRange(0.0f, 10.0f), 0.0f,
                                            "Makes the crest factor shape asymmetrical around zero crossings"};
    static const ParameterInfo rubidiumTone{"rubidiumTone", "Rubidium Tone", ParamUnits::hz, makeSkewedRange(4.0f, 100.0f, 0.5f), 5.0f,
                                            "Effectively a high pass, more easily increase crest factor increase to the signal for higher frequencies"};
    static const ParameterInfo rubidiumBias{"rubidiumBias", "Rubidium Bias", ParamUnits::none, makeRange(0.0f, 1.0f), 0.0f,
                                            "Applies DC offset before the distortion, creating significant even harmonics"};

    // phase distortion
    static const ParameterInfo phaseAmount{"phaseAmount", "Phase Distortion", ParamUnits::percent, makeRange(0.0f, 100.0f), 0.0f,
                                           "Phase Distortion Strength, self modulating signal modulating a tiny delay line"};
    static const ParameterInfo phaseDistTone{"phaseDistTone", "Phase Dist Tone", ParamUnits::hz, makeSkewedRange(20.0f, 20000.0f, 0.25f), 355.0f,
                                             "Low pass filter, exclude frequencies above this from self-modulating"};
    static const ParameterInfo phaseDistStereo{"phaseDistStereo", "Phase Dist Stereo", ParamUnits::none, makeRange(0.0f, 1.0f), 0.0f,
                                               "Make L and R channels self-modulate independently. By default, both channels are summed before modulating."};
    static const ParameterInfo phaseRectify{"phaseRectify", "Phase Dist Rectify", ParamUnits::none, makeRange(0.0f, 1.0f), 0.0f,
                                            "Rectifies the waveform in the modulation path, causing the resulting phase distortion to contain odd harmonics only"};
    static const ParameterInfo phaseShift{"phaseShift", "Phase Dist Shift", ParamUnits::none, makeRange(-1.0f, 1.0f), 0.0f,
                                          "Frequency shift the modulation path. Useful at small values for additional movement"};

    // tube distortion
    static const ParameterInfo tubeAmount{"tubeAmount", "Tube Saturation", ParamUnits::percent, makeRange(0.0f, 100.0f), 0.0f,
                                          "Tube Saturation Strength, based on an empirical emulation of Class A tube distortion by Will Pirkle"};
    static const ParameterInfo tubeTone{"tubeTone", "Tube Tone", ParamUnits::none, makeRange(0.0f, 1.0f), 1.0f,
                                        "Brightness of the tube"};
    static const ParameterInfo tubeBias{"tubeBias", "Tube Bias", ParamUnits::none, makeRange(0.0f, 1.0f), 0.0f,
                                        "Offsets the waveform into the tube curve, adding even harmonics"};
    static const ParameterInfo jeffAmount{"jeffAmount", "Tube Jeff Amt", ParamUnits::none, makeRange(0.0f, 100.0f), 0.0f,
                                          "Adds an extra aggressive sine waveshaping on top of the signal"};

    // tape hysteresis
    static const ParameterInfo tapeBias{"tapeBias", "Tape Bias", ParamUnits::none, makeRange(0.0f, 1.0f), 0.0f,
                                        "Applies DC biasing to the signal before distortion, adding even harmonics"};
    static const ParameterInfo tapeDrive{"tapeDrive", "Tape Drive", ParamUnits::none, makeRange(0.0f, 1.0f), 0.0f,
                                         "Tape Saturation Strength, based on a realistic magnetic tape hysteresis model"};
    static const ParameterInfo tapeWidth{"tapeWidth", "Tape Age", ParamUnits::none, makeRange(0.0f, 1.0f), 0.3f,
                                         "Ages the tape, making low end subtly more pronounced by widening the hysteresis curve"};

    // sizzle
    static const ParameterInfo sizzleAmount{"sizzleAmount", "Sizzle Amt", ParamUnits::none, makeRange(0.0f, 100.0f), 5.0f,
                                            "Sizzle Noise strength, adds noise in zero crossings of the signal."};
    static const ParameterInfo sizzleFrequency{"sizzleFrequency", "Sizzle Freq", ParamUnits::none, makeSkewedRange(20.0f, 20000.0f, 0.25f), 4000.0f,
                                               "Frequency the sizzle noise sits around"};
    static const ParameterInfo sizzleQ{"sizzleQ", "Sizzle Q", ParamUnits::none, makeRange(0.1f, 1.5f), 1.0f,
                                       "Size of sizzle's amplitude band width around the zero crossing"};

    static const ParameterInfo fizzAmount{"fizzAmount", "Fizz Amt", ParamUnits::none, makeRange(0.0f, 100.0f), 5.0f,
                                          "Amount of noise added to the signal. Noise folds back past halfway"};

    // erosion
    static const ParameterInfo erosionAmount{"erosionAmount", "Erosion Amt", ParamUnits::none, makeRange(0.0f, 100.0f), 3.0f,
                                             "Erosion noise strength. Applies noise similar to Ableton's erosion device"};
    static const ParameterInfo erosionFrequency{"erosionFrequency", "Noise Freq", ParamUnits::hz, makeSkewedRange(20.0f, 20000.0f, 0.25f), 400.0f,
                                                "Frequency the erosion noise resonates at."};
    static const ParameterInfo erosionQ{"erosionQ", "Erosion Q", ParamUnits::none, makeRange(0.1f, 1.5f), 1.0f,
                                        "Erosion Filter Q factor, resonance amount"};

    static const ParameterInfo downsampleFreq{"downsampleFreq", "Dwnsmpl Freq", ParamUnits::hz, makeSkewedRange(200.0f, 40000.0f, 0.25f), 40000.0f,
                                              "The new sample rate the signal is crushed to"};
    static const ParameterInfo downsampleMix{"downsampleMix", "Dwnsmpl Mix", ParamUnits::none, makeRange(0.0f, 1.0f), 1.0f,
                                             "Dry / Wet blend the digitised and original signal"};
    static const ParameterInfo bitReduction{"bitReduction", "Dwnsmpl Bits", ParamUnits::none, makeRange(1.0f, 32.0f), 32.0f,
                                            "Bit depth the signal is quantised to"};

    // allpass
    static const ParameterInfo allPassFreq{"allPassFreq", "AllPass Frequency", ParamUnits::hz, makeSkewedRange(20.0f, 20000.0f, 0.25f), 85.0f,
                                           "The frequency the allpass filters center around"};
    static const ParameterInfo allPassQ{"allPassQ", "AllPass Q", ParamUnits::none, makeRange(0.01f, 1.41f), 0.4f,
                                        "Resonance of the allpass filters"};
    static const ParameterInfo allPassAmount{"allPassAmount", "AllPass Number", ParamUnits::none, makeRange(0.0f, 50.0f), 10.0f,
                                             "# of Allpasses in the stack, higher number results in transient smearing"};

    // slew
    static const ParameterInfo alphaParam{"alphaParam", "Strength", ParamUnits::none, makeRange(0.0f, 1.0f), 1.0f,
                                          "Slew distortion strength, a model that modifies \"rate of change\" of signal. If you can't hear changes, set this to max."};
    static const ParameterInfo slewSpeed{"slewSpeed", "Slew Tone", ParamUnits::none, makeRange(0.0f, 1.0f), 0.5f,
                                         "Slew speed, controls maximum speed that signal can track onto, acting as a fake lowpass control"};
    static const ParameterInfo directionality{"directionality", "Slew Bias", ParamUnits::none, makeRange(-1.0f, 1.0f), 0.0f,
                                              "Bias the slew limiting towards rising or falling edges"};
    // waveshape

    static const ParameterInfo waveshapeDrive{"waveshapeDrive", "Waveshape Drive", ParamUnits::db, makeRange(-24.0f, 24.0f), 0.0f,
                                              "Gain into the waveshaper, pushing the signal further along the curve"};
    static const ParameterInfo waveshapeX{"waveshapeX", "Waveshape X", ParamUnits::none, makeRange(0.0f, 1.0f), 0.5f,
                                          "Across the waveshape map, where shapes with similar harmonics sit together. The pad blends the nearest few"};
    static const ParameterInfo waveshapeY{"waveshapeY", "Waveshape Y", ParamUnits::none, makeRange(0.0f, 1.0f), 0.5f,
                                          "Up the waveshape map, where shapes with similar harmonics sit together. The pad blends the nearest few"};
    static const ParameterInfo waveshapeSmooth{"waveshapeSmooth", "Waveshape Smooth", ParamUnits::percent, makeRange(0.0f, 100.0f), 0.0f,
                                               "Rounds off sharp corners in the curve by averaging it over a wider area, without filtering the audio"};
    static const ParameterInfo waveshapeBias{"waveshapeBias", "Waveshape Bias", ParamUnits::none, makeRange(-1.0f, 1.0f), 0.0f,
                                             "Offsets the signal into the curve for even harmonics. The offset's own output is taken back out, so silence stays silent"};
    static const ParameterInfo waveshapeAsym{"waveshapeAsym", "Waveshape Asym", ParamUnits::percent, makeRange(0.0f, 100.0f), 0.0f,
                                             "Skews the signal before the curve: the negative side flattens out while the positive side accelerates up"};

    static const ParameterInfo slewType{"slewType", "Slew Type", ParamUnits::category, makeSteppedRange(0.0f, 2.0f), 0.0f,
                                        "Select a slew limiting algorithm to use. Non-standard slew algorithms are present after the first one."};
}

inline juce::ParameterID paramIdFor (SlotId slot, const ParamIDs::ParameterInfo& descriptor)
{
    return { slot.prefix() + "_" + descriptor.id.toString(), descriptor.versionHint };
}
