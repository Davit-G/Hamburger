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

using RoutingOrder = std::array<ModuleId, static_cast<size_t>(ModuleId::count)>;

// 16 items fit in an atomic if each item is represented as a uint4 so we can just cheat threading
static constexpr size_t routingBitsPerSlot = 4;

static_assert (static_cast<size_t>(ModuleId::count) <= 64 / routingBitsPerSlot);

inline constexpr juce::uint64 packRouting(const RoutingOrder& order)
{
    juce::uint64 packed = 0;

    for (size_t i = 0; i < order.size(); ++i)
        packed |= static_cast<juce::uint64>(order[i]) << (i * routingBitsPerSlot);

    return packed;
}

inline constexpr RoutingOrder unpackRouting(juce::uint64 packed)
{
    RoutingOrder order {};

    for (size_t i = 0; i < order.size(); ++i)
        order[i] = static_cast<ModuleId>((packed >>(i * routingBitsPerSlot)) & 0xf);

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
    {ModuleId::preDistortion, "preDistortion", "PRE", 1, true},
    {ModuleId::main, "main", "MAIN", 4, true},
    {ModuleId::postDistortion, "postDistortion", "POST", 1, false},
    {ModuleId::postEmphasis, "postEmphasis", "POST-EMPH", 0},
    {ModuleId::postClip, "postClip", "CLIP", 1, true},
};

// if we are missing a module ID this will catch it
static_assert (std::size(pluginModules) == static_cast<size_t>(ModuleId::count));

// on compile time, generate the default routing we wanna use
static constexpr RoutingOrder defaultRouting = [](){
    RoutingOrder chain {};

    for (size_t i = 0; i < std::size(pluginModules); ++i)
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
        auto p = juce::String(pluginModules[(size_t) module].key);
        return sub == 0 ? p : p + "_slot" + juce::String(sub);
    }

    juce::String displayNamePrefix() const
    {
        const auto& m = pluginModules[(size_t) module];
        auto p = juce::String(m.displayName);

        // numbered from 1, and only when the module actually has more than one slot
        return m.slotCount > 1 ? p + " " + juce::String(sub + 1) : p;
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
        ValueType unit = ValueType::none;
        juce::NormalisableRange<float> range;
        float defaultValue = 0.0f;
        juce::String paramTooltip;
        int versionHint = 1; // see VERSION HINTS above. never change a released parameter's

        juce::String getParamID() const { return id.toString(); }
        juce::ParameterID getParameterID() const { return { id.toString(), versionHint }; }
    };

    static constexpr int choiceSlots = 16;

    inline juce::StringArray withReservedSlots(juce::StringArray names)
    {
        jassert(names.size() <= choiceSlots);

        for (int i = names.size(); i < choiceSlots; ++i)
            names.add("RESERVED " + juce::String(i + 1));

        return names;
    }

    /*  The names every type goes by on screen: in its box's type menu, which reads them from here, and in the FX order.
        Only renamed or added to the end, never reordered or removed: a preset stores the index. */
    static const PanelInfo distortionTypes { juce::StringArray({"GRILL", "TUBE", "PHASE", "RUBIDIUM", "TAPE", "SLEW", "WAVESHAPE"
#if JUCE_DEBUG
                                                                  , "DLL" // debug only, always last. hot reloads a dylib, see hotdsp/HotDsp.h
#endif
                                                                  }) };
    static const PanelInfo compTypes { juce::StringArray({"STEREO", "MB", "MS", "TYPE A", "TRANSIENT", "MB TRANSIENT", "OPTO", "MB OPTO"}) };
    static const PanelInfo noiseTypes { juce::StringArray({"SIZZLE", "EROSION", "BIT", "GATE", "FIZZ"}) };
    static const PanelInfo clipTypes { juce::StringArray({"CLIP", "LIMIT", "FOLD1", "FOLD2"}) };
    static const PanelInfo preFxTypes { juce::StringArray({"ALLPASS", "GRUNGE", "HARMONIC", "FORMANT", "FREQ SHIFT"}) };
    // keep in line with EmphasisFilter::Mode
    static const PanelInfo emphasisTypes { juce::StringArray({"EQ", "TILT"}) };

    // keep in step with MainRouting::Routing
    static const PanelInfo routingTypes { juce::StringArray({"STACK", "MULTIBAND", "EXCITER"}) };

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
        case ModuleId::postClip: return &clipTypes;
        case ModuleId::preEmphasis:
        case ModuleId::postEmphasis:
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
    static constexpr int numGlobalMacros = 2;

    inline const ParameterInfo globalMacro1{"macro1", "Macro 1", ValueType::percent, makeRange(0.0f, 100.0f), 0.0f,
                                            "Assignable macro knob"};
    inline const ParameterInfo globalMacro2{"macro2", "Macro 2", ValueType::percent, makeRange(0.0f, 100.0f), 0.0f,
                                            "Assignable macro knob"};

    inline const ParameterInfo* const globalMacros[numGlobalMacros] {
        &globalMacro1, &globalMacro2
    };

    static_assert(sizeof(globalMacros) / sizeof(ParameterInfo*) == numGlobalMacros);

    // a macro's name once renamed, kept on the state so it's saved with the preset. unset, it goes by its own
    inline juce::Identifier macroNameProperty(int macro) { return globalMacros[macro]->getParamID() + "Name"; }

    // which macro a parameter is, -1 for the rest
    inline int macroIndexOf(const ParameterInfo& info)
    {
        for (int i = 0; i < numGlobalMacros; ++i)
            if (info.id == globalMacros[i]->id)
                return i;

        return -1;
    }

    /*  The mod matrix's modulators, each any of the types, with every type's settings kept whichever one it is. Types
        are only ever added to the end, the reserved slots after them are for the likes of random generators. */
    static constexpr int numModulators = 3;
    static const PanelInfo modulatorTypes { juce::StringArray({"LFO", "ENVELOPE", "AUDIO", "RANDOM"}) };
    enum ModulatorType { lfo, envelope, audio, random };
    enum ModulatorParam { modRate, modShape, modAttack, modRelease, modGain, modRectify, modRateWide, modStereo, modSyncRate, modRandomType, modSmooth, numModulatorParams };
    static const juce::StringArray randomTypes { "S&H", "GAUSSIAN", "PERLIN" };
    enum RandomType { sampleAndHold, gaussian, perlin };
    enum ModulatorToggle { modOn, modWideRange, modSawDown, modSync, numModulatorToggles };

    // modulator slot 0 to 2
    inline const ParameterInfo& modulatorParam(int slot, int which)
    {
        static const auto table = []
        {
            std::array<std::array<ParameterInfo, numModulatorParams>, numModulators> built;

            for (int s = 0; s < numModulators; ++s)
            {
                const auto id = "mod" + juce::String(s + 1), name = "Mod " + juce::String(s + 1);

                built[(size_t) s] = {{
                    { id + "Rate", name + " Rate", ValueType::hz, makeSkewedRange(0.01f, 40.0f, 0.3f), 1.0f, "How fast the LFO cycles" },
                    { id + "Shape", name + " Shape", ValueType::none, makeRange(0.0f, 1.0f), 0.0f, "Morphs the LFO from sine to triangle to saw to square" },
                    { id + "Attack", name + " Attack", ValueType::ms, makeSkewedRange(0.0f, 500.0f, 0.3f), 10.0f, "How fast the envelope rises. At 0ms it jumps straight there" },
                    { id + "Release", name + " Release", ValueType::ms, makeSkewedRange(0.0f, 2000.0f, 0.3f), 150.0f, "How fast the envelope falls. At 0ms it drops straight away" },
                    { id + "Gain", name + " Gain", ValueType::db, makeRange(-24.0f, 24.0f), 0.0f, "Gain on the input before it's followed or used as the modulation" },
                    { id + "Rectify", name + " Rectify", ValueType::percent, makeRange(0.0f, 100.0f), 0.0f,
                      "Folds the negative half up. At 100% it follows the audio's level" },
                    { id + "RateWide", name + " Rate Wide", ValueType::hz, makeSkewedRange(0.01f, 22000.0f, 0.15f), 1.0f,
                      "LFO rate up into audio rates" },
                    { id + "Stereo", name + " Stereo", ValueType::degrees, makeRange(0.0f, 180.0f), 0.0f,
                      "Offsets the right channel's LFO from the left's, moving the two sides apart" },
                    { id + "SyncRate", name + " Sync Rate", ValueType::beats, makeSteppedRange(0.0f, numBeatDivisions - 1.0f), 9.0f,
                      "How long the LFO takes per cycle, locked to the host's tempo" },
                    { id + "RandomType", name + " Random Type", ValueType::category, makeSteppedRange(0.0f, randomTypes.size() - 1.0f), 0.0f,
                      "A new value each cycle, picked evenly or near the middle, or a smooth noise" },
                    { id + "Smooth", name + " Smooth", ValueType::percent, makeRange(0.0f, 100.0f), 0.0f,
                      "Glides between the random values, over up to a whole cycle" },
                }};
            }

            return built;
        }();

        return table[(size_t) slot][(size_t) which];
    }

    // a modulator's switches: on, so it can be switched off without losing what it's routed to, and the LFO's options
    inline const ParameterInfo& modulatorToggle(int slot, int which)
    {
        static const auto table = []
        {
            std::array<std::array<ParameterInfo, numModulatorToggles>, numModulators> built;

            for (int s = 0; s < numModulators; ++s)
            {
                const auto id = "mod" + juce::String(s + 1), name = "Mod " + juce::String(s + 1);
                const auto range = makeSteppedRange(0.0f, 1.0f);

                built[(size_t) s] = {{
                    { id + "On", name + " On", ValueType::none, range, 1.0f, "Switches this modulator off, keeping where it's routed" },
                    { id + "WideRange", name + " Bigger Freq Range", ValueType::none, range, 0.0f, "Lets the LFO run up to 22kHz" },
                    { id + "SawDown", name + " Saw Down", ValueType::none, range, 0.0f, "Ramps the saw down instead of up" },
                    { id + "Sync", name + " BPM Sync", ValueType::none, range, 0.0f, "Locks the LFO's rate to the host's tempo" },
                }};
            }

            return built;
        }();

        return table[(size_t) slot][(size_t) which];
    }

    // an envelope then LFOs by default
    inline const ParameterInfo& modulatorType(int slot)
    {
        static const auto table = []
        {
            std::array<ParameterInfo, numModulators> built;

            for (int s = 0; s < numModulators; ++s)
                built[(size_t) s] = { "mod" + juce::String(s + 1) + "Type", "Mod " + juce::String(s + 1) + " Type", ValueType::category,
                                      makeSteppedRange(0.0f, choiceSlots - 1.0f), (float) (s == 0 ? envelope : lfo), "What this modulator is" };

            return built;
        }();

        return table[(size_t) slot];
    }

    // how the main stage arranges its slots, plus the controls the individual routings need
    inline const ParameterInfo mainEnabled{"mainOn", "Main On", ValueType::none, makeSteppedRange(0.0f, 1.0f), 0.0f,
                                           "Switches the whole main section off, every slot and the stack or bands around them"};
    inline const ParameterInfo mainRouting{"mainRouting", "Routing", ValueType::category, makeSteppedRange(0.0f, choiceSlots - 1.0f), 0.0f,
                                           "How the distortion slots are arranged"};

    // the multiband routing's own setting, kept while another routing is on so coming back finds it as it was
    inline const ParameterInfo bandCount{"bandCount", "Band Count", ValueType::category, makeSteppedRange(2.0f, 4.0f), 2.0f,
                                         "How many bands the multiband routing splits into"};

    inline const ParameterInfo crossoverLow{"crossoverLow", "Crossover Low", ValueType::hz, makeSkewedRange(20.0f, 20000.0f, 0.25f), 200.0f,
                                            "Split between the first and second bands, and the exciter's split"};
    inline const ParameterInfo crossoverMid{"crossoverMid", "Crossover Mid", ValueType::hz, makeSkewedRange(20.0f, 20000.0f, 0.25f), 1000.0f,
                                            "Split point between the second and third band"};
    inline const ParameterInfo crossoverHigh{"crossoverHigh", "Crossover High", ValueType::hz, makeSkewedRange(20.0f, 20000.0f, 0.25f), 5000.0f,
                                             "Split point between the third and fourth band"};

    inline const ParameterInfo stackCount{"stackCount", "Stack Count", ValueType::category, makeSteppedRange(1.0f, 4.0f), 1.0f,
                                          "The number of distortions applied in a row"};
    inline const ParameterInfo stackFlip{"stackFlip", "Stack Flip Polarity", ValueType::none, makeSteppedRange(0.0f, 1.0f), 1.0f,
                                         "Flips polarity between stack stages, evening out asymmetric drive"};
    inline const ParameterInfo stackGain{"stackGain", "Stack Stage Gain", ValueType::db, makeRange(-24.0f, 24.0f), -8.0f,
                                         "Increase / decrease gain at each stage"};
    inline const ParameterInfo stackMix{"stackMix", "Stack Dry/Wet", ValueType::percent, makeRange(0.0f, 100.0f), 100.0f,
                                        "Dry / wet of the whole stack, on top of each stage's own dry / wet"};

    //  keep in line with MainRouting::StackFilter
    static const PanelInfo stackFilterTypes { juce::StringArray({"NO FILTER", "ALLPASS", "HILBERT", "BANDPASS", "NOTCH"}) };
    inline const ParameterInfo stackFilter{"stackFilter", "Stack Filter", ValueType::category, makeSteppedRange(0.0f, choiceSlots - 1.0f), 0.0f,
                                           "Choose a filter between stack stages"};
    inline const ParameterInfo stackFilterFreq{"stackFilterFreq", "Stack Filter Freq", ValueType::hz, makeSkewedRange(20.0f, 20000.0f, 0.25f), 1000.0f,
                                               "Cutoff / center frequency of filter effect"};
    inline const ParameterInfo stackFilterQ{"stackFilterQ", "Stack Filter Q", ValueType::none, makeSkewedRange(0.1f, 10.0f, 0.3f), 0.707f,
                                            "How narrow the bandpass or notch between stages is"};
    inline const ParameterInfo stackRotation{"stackRotation", "Stack Rotation", ValueType::degrees, makeRange(0.0f, 90.0f), 45.0f,
                                             "How far to rotate phase of all frequencies every stage"};

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
                                               "Boosts or cuts the lows into the distortion, the reverse after"};
    inline const ParameterInfo emphasisHighGain{"emphasisHighGain", "Emphasis Hi Gain", ValueType::db, makeRange(-18.0f, 18.0f), 0.0f,
                                                "Boosts or cuts the highs into the distortion, the reverse after"};
    inline const ParameterInfo emphasisLowFreq{"emphasisLowFreq", "Emphasis Low Frequency", ValueType::hz, makeSkewedRange(20.0f, 20000.0f, 0.25f), 62.0f,
                                               "Frequency of the low emphasis filter"};
    inline const ParameterInfo emphasisHighFreq{"emphasisHighFreq", "Emphasis Hi Frequency", ValueType::hz, makeSkewedRange(20.0f, 20000.0f, 0.25f), 9000.0f,
                                                "Frequency of the high emphasis filter"};

    inline const ParameterInfo emphasisOn{"emphasisOn", "Emphasis EQ On", ValueType::none, makeSteppedRange(0.0f, 1.0f), 1.0f,
                                          "Enables the emphasis EQ around the distortion"};
    inline const ParameterInfo emphasisType{"emphasisType", "Emphasis Type", ValueType::category, makeSteppedRange(0.0f, choiceSlots - 1.0f), 0.0f,
                                            "Two bands of emphasis, or one tilt across the whole spectrum"};
    inline const ParameterInfo emphasisTilt{"emphasisTilt", "Emphasis Tilt", ValueType::db, makeRange(-18.0f, 18.0f), 0.0f,
                                            "Tilts the spectrum after the distortion, and the other way before it"};
    inline const ParameterInfo hamburgerEnabled{"hamburgerEnabled", "Hamburger Enabled", ValueType::none, makeSteppedRange(0.0f, 1.0f), 1.0f,
                                                "Bypasses the entire plugin when off"};

    // the start page. these scale other parameters on their way to the audio without moving their knobs, see scaleFor
    inline const ParameterInfo globalDrive{"globalDrive", "Global Drive", ValueType::percent, makeRange(0.0f, 200.0f), 100.0f,
                                           "Scales every distortion's drive, from none up to double"};
    inline const ParameterInfo compAmount{"compAmount", "Compressor Amount", ValueType::percent, makeRange(0.0f, 200.0f), 100.0f,
                                          "Scales compressor threshold and ratio, or transient attack and sustain"};
    inline const ParameterInfo eqStrength{"eqStrength", "EQ Strength", ValueType::percent, makeRange(-200.0f, 200.0f), 100.0f,
                                          "Scales the emphasis gains, or the tilt. Below 0% flips them"};
    inline const ParameterInfo noiseAmount{"noiseAmount", "Noise Amount", ValueType::percent, makeRange(0.0f, 200.0f), 100.0f,
                                           "Scales how much noise the noise box adds, whichever type it is"};
    inline const ParameterInfo preFxAmount{"preFxAmount", "Pre FX Amount", ValueType::percent, makeRange(0.0f, 200.0f), 100.0f,
                                           "Scales how strong the pre fx box is, whichever type it is"};

    inline const ParameterInfo oversamplingFactor{"oversamplingFactor", "Oversampling Factor", ValueType::oversample, makeSteppedRange(0.0f, 2.0f), 0.0f,
                                                  "Upsamples to reduce aliasing. Heavy on CPU at higher values"};

    // compressor
    inline const ParameterInfo compSpeed{"compSpeed", "Comp Speed", ValueType::ms, makeSkewedRange(0.0f, 400.0f, 0.25f), 100.0f,
                                         "Attack and release together. At 0ms it acts as a clipper"};
    inline const ParameterInfo MBCompSpeed{"MBCompSpeed", "Multiband Comp Speed", ValueType::ms, makeSkewedRange(0.0f, 400.0f, 0.25f), 100.0f,
                                           "Attack and release together. At 0ms it acts as a clipper"};
    inline const ParameterInfo MSCompSpeed{"MSCompSpeed", "Mid Side Comp Speed", ValueType::ms, makeSkewedRange(0.0f, 400.0f, 0.25f), 100.0f,
                                           "Attack and release together. At 0ms it acts as a clipper"};
    inline const ParameterInfo TypeACompSpeed{"TypeACompSpeed", "Type A Comp Speed", ValueType::ms, makeSkewedRange(0.0f, 400.0f, 0.25f), 100.0f,
                                              "Attack and release together. At 0ms it acts as a clipper"};

    inline const ParameterInfo compBandTilt{"compBandTilt", "Comp Band Tilt", ValueType::db, makeRange(-20.0f, 20.0f), 0.0f,
                                            "Tilts the threshold across bands, compressing lows or highs harder"};
    inline const ParameterInfo compStereoLink{"compStereoLink", "Stereo Link", ValueType::percent, makeRange(0.0f, 100.0f), 100.0f,
                                              "How much L and R share gain reduction. At 0% each side is separate"};
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
                                         "Tilts Type A's output across bands, towards the lows or the highs"};

    inline const ParameterInfo transientAttack{"transientAttack", "Transient Attack", ValueType::db, makeRange(-24.0f, 24.0f), 6.0f,
                                               "Boosts or cuts the start of every hit"};
    inline const ParameterInfo transientSustain{"transientSustain", "Transient Sustain", ValueType::db, makeRange(-24.0f, 24.0f), 0.0f,
                                                "Boosts or cuts what rings out after every hit"};
    inline const ParameterInfo transientSpeed{"transientSpeed", "Transient Speed", ValueType::ms, makeSkewedRange(0.0f, 600.0f, 0.25f), 40.0f,
                                              "How long a hit's start lasts. At 0ms it shapes every cycle"};
    inline const ParameterInfo transientLink{"transientLink", "Transient Link", ValueType::percent, makeRange(0.0f, 100.0f), 100.0f,
                                             "How much L and R share their shaping. At 0% each side is separate"};
    inline const ParameterInfo transientTilt{"transientTilt", "Transient Tilt", ValueType::percent, makeRange(-100.0f, 100.0f), 0.0f,
                                             "Shifts attack and sustain towards the lows or the highs"};

    // gate (noise distortion)
    inline const ParameterInfo gateAmt{"gateAmt", "Gate Amt", ValueType::none, makeRange(0.0f, 1.0f), 0.0f,
                                       "The minimum threshold at which gate will let audio pass through"};
    inline const ParameterInfo gateSmooth{"gateSmooth", "Gate Smooth", ValueType::percent, makeRange(0.0f, 100.0f), 0.0f,
                                          "Softens the gate's edge, easing in and out instead of snapping"};

    // grunge
    inline const ParameterInfo grungeAmt{"grungeAmt", "Grunge Amt", ValueType::none, makeRange(0.0f, 1.0f), 0.0f,
                                         "Strength of grunge distortion applied"};
    inline const ParameterInfo grungeTone{"grungeTone", "Grunge Tone", ValueType::none, makeRange(0.0f, 1.0f), 0.5f,
                                          "Chooses suitable frequency to resonate at"};

    inline const ParameterInfo harmonicAmount{"harmonicAmount", "Harmonic Amount", ValueType::percent, makeRange(0.0f, 100.0f), 50.0f,
                                              "Level of the harmonic stack put back on top of the input"};
    inline const ParameterInfo harmonicRatio{"harmonicRatio", "Harmonic Ratio", ValueType::ratio, makeSteppedRange(0.0f, (float) (numHarmonicRatios - 1)), 3.0f,
                                             "Which step of the pitch the loop locks to comes out. Under 1 is a subharmonic, over it a harmonic"};
    inline const ParameterInfo formantShift{"formantShift", "Formant Shift", ValueType::none, makeRange(-12.0f, 12.0f), 0.0f,
                                            "Moves the input's formants up or down, in semitones, leaving its pitch where it is"};
    inline const ParameterInfo formantDepth{"formantDepth", "Formant Depth", ValueType::percent, makeRange(0.0f, 200.0f), 100.0f,
                                            "How strongly the formants are put back. 0\\% flattens them out, past 100\\% exaggerates them"};
    inline const ParameterInfo freqShift{"freqShift", "Freq Shift", ValueType::none, makeRange(-1.0f, 1.0f), 0.0f,
                                         "Moves every frequency up or down by the same amount, so the harmonics stop lining up. Small values add movement"};
    inline const ParameterInfo freqShiftFeedback{"freqShiftFeedback", "Freq Shift Feedback", ValueType::percent, makeRange(0.0f, 100.0f), 0.0f,
                                                 "Feeds the shifted sound back in, so it's shifted again every pass and spirals up or down"};
    inline const ParameterInfo freqShiftDelay{"freqShiftDelay", "Freq Shift Delay", ValueType::delay, makeSkewedRange(0.0f, 50.0f, 0.4f), 0.0f,
                                              "How long the shifted sound takes to come back round, from the very next sample up. Longer turns the spiral into echoes"};
    inline const ParameterInfo formantResonance{"formantResonance", "Formant Resonance", ValueType::percent, makeRange(-100.0f, 100.0f), 0.0f,
                                                "Narrows the formants put back into ringing peaks, or widens them into a smoother body"};

    // clipper
    inline const ParameterInfo postClipGain{"postClipGain", "SoftClip Gain", ValueType::db, makeRange(-18.0f, 18.0f), 0.0f,
                                            "The gain of the audio applied into the clipper"};
    inline const ParameterInfo postClipKnee{"postClipKnee", "SoftClip Knee", ValueType::db, makeRange(0.0f, 24.0f), 2.0f,
                                            "Knee width centred on 0dB, bending below and holding above"};
    inline const ParameterInfo postClipTime{"postClipTime", "Limiter Time", ValueType::ms, makeSkewedRange(1.0f, 1000.0f, 0.3f), 50.0f,
                                            "How fast the limiter lets go. It sees each peak 1.5ms early"};
    inline const ParameterInfo postClipFoldFreq{"postClipFoldFreq", "Fold Highpass", ValueType::hz, makeSkewedRange(20.0f, 20000.0f, 0.25f), 2500.0f,
                                                "Only the clipped top end above this is folded back"};

    // grill saturation
    inline const ParameterInfo saturationAmount{"saturationAmount", "Grill Saturation", ValueType::percent, makeRange(0.0f, 100.0f), 0.0f,
                                                "Grill strength, an all-rounder that folds back at high levels"};
    inline const ParameterInfo diode{"diode", "Grill Diode", ValueType::none, makeRange(0.0f, 100.0f), 0.0f,
                                     "Activates an extremely aggressive clipping"};
    inline const ParameterInfo fold{"fold", "Grill Fold", ValueType::none, makeRange(0.0f, 100.0f), 0.0f,
                                    "Wavefolding, reflects the signal once it goes over 0db"};
    inline const ParameterInfo grillBias{"grillBias", "Grill Bias", ValueType::none, makeRange(0.0f, 1.0f), 0.0f,
                                         "Strength of DC offset before distortion. Adds even harmonics"};
    inline const ParameterInfo grillDcTiming{"grillDcTiming", "Grill DC Env Speed", ValueType::ms, makeSkewedRange(0.0f, 300.0f, 0.25f), 50.0f,
                                             "How fast the DC bias level jumps. Does nothing if DC Bias is 0"};

    // matrix distortion
    inline const ParameterInfo matrix1{"matrix1", "Matrix #1", ValueType::none, makeRange(0.0f, 1.0f), 0.0f,
                                       "Drive into the matrix waveshaper"};
    inline const ParameterInfo matrix2{"matrix2", "Matrix #2", ValueType::none, makeRange(0.0f, 1.0f), 0.0f,
                                       "Add gritty ripple to the wave"};
    inline const ParameterInfo matrix3{"matrix3", "Matrix #3", ValueType::none, makeRange(0.0f, 1.0f), 0.0f,
                                       "Bounce / Wavefold"};
    inline const ParameterInfo matrix4{"matrix4", "Matrix #4", ValueType::none, makeRange(0.0f, 1.0f), 0.0f,
                                       "Crushing / digital stepping"};
    inline const ParameterInfo matrix5{"matrix5", "Matrix #5", ValueType::none, makeRange(0.0f, 1.0f), 0.0f,
                                       "Morphs between the filter shapes feeding the waveshaper"};
    inline const ParameterInfo matrix6{"matrix6", "Matrix #6", ValueType::none, makeRange(0.0f, 1.0f), 0.0f,
                                       "Cutoff frequency of the matrix filters"};
    inline const ParameterInfo matrix7{"matrix7", "Matrix #7", ValueType::none, makeRange(0.0f, 1.0f), 0.0f,
                                       "Currently unused"};
    inline const ParameterInfo matrix8{"matrix8", "Matrix #8", ValueType::none, makeRange(0.0f, 1.0f), 0.0f,
                                       "Currently unused"};
    inline const ParameterInfo matrix9{"matrix9", "Matrix #9", ValueType::none, makeRange(0.0f, 1.0f), 1.0f,
                                       "Blends the filtered signal against the dry one before shaping"};

    // rubidium
    inline const ParameterInfo rubidiumAmount{"rubidiumAmount", "Rubidium Saturation", ValueType::percent, makeRange(0.0f, 100.0f), 5.0f,
                                              "Rubidium Strength, unique distortion that adds crest factor to signal"};
    inline const ParameterInfo rubidiumMojo{"rubidiumMojo", "Rubidium Mojo", ValueType::none, makeRange(0.0f, 100.0f), 5.0f,
                                            "Increases crest factor of signal"};
    inline const ParameterInfo rubidiumAsym{"rubidiumAsym", "Rubidium Asymmetry", ValueType::none, makeRange(0.0f, 10.0f), 0.0f,
                                            "Makes the crest factor shape asymmetrical around zero crossings"};
    inline const ParameterInfo rubidiumTone{"rubidiumTone", "Rubidium Tone", ValueType::hz, makeSkewedRange(4.0f, 100.0f, 0.5f), 5.0f,
                                            "Acts as a high pass, raising the crest factor of the highs"};
    inline const ParameterInfo rubidiumBias{"rubidiumBias", "Rubidium Bias", ValueType::none, makeRange(0.0f, 1.0f), 0.0f,
                                            "Adds DC offset before the distortion for even harmonics"};

    // phase distortion
    inline const ParameterInfo phaseAmount{"phaseAmount", "Phase Distortion", ValueType::percent, makeRange(0.0f, 100.0f), 0.0f,
                                           "Phase distortion strength, the signal modulating a tiny delay line"};
    inline const ParameterInfo phaseDistTone{"phaseDistTone", "Phase Dist Tone", ValueType::hz, makeSkewedRange(20.0f, 20000.0f, 0.25f), 355.0f,
                                             "Low pass filter, exclude frequencies above this from self-modulating"};
    inline const ParameterInfo phaseDistStereo{"phaseDistStereo", "Phase Dist Stereo", ValueType::none, makeRange(0.0f, 1.0f), 0.0f,
                                               "L and R modulate themselves separately instead of summed"};
    inline const ParameterInfo phaseRectify{"phaseRectify", "Phase Dist Rectify", ValueType::none, makeRange(0.0f, 1.0f), 0.0f,
                                            "Rectifies the modulation path, so only odd harmonics are made"};
    inline const ParameterInfo phaseShift{"phaseShift", "Phase Dist Shift", ValueType::none, makeRange(-1.0f, 1.0f), 0.0f,
                                          "Frequency shifts the modulation path. Small values add movement"};

    // tube distortion
    inline const ParameterInfo tubeAmount{"tubeAmount", "Tube Saturation", ValueType::percent, makeRange(0.0f, 100.0f), 0.0f,
                                          "Tube strength, from Will Pirkle's class A tube model"};
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
                                         "Tape strength, from a magnetic tape hysteresis model"};
    inline const ParameterInfo tapeWidth{"tapeWidth", "Tape Age", ValueType::none, makeRange(0.0f, 1.0f), 0.3f,
                                         "Ages the tape, widening the hysteresis for more low end"};

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
    inline const ParameterInfo alphaParam{"alphaParam", "Slew Drive", ValueType::db, makeRange(0.0f, 24.0f), 6.0f,
                                          "How hard the audio is pushed into the slew limit. Louder slews more"};
    inline const ParameterInfo slewSpeed{"slewSpeed", "Slew Tone", ValueType::hz, makeSkewedRange(20.0f, 20000.0f, 0.23f), 2000.0f,
                                         "A -12dB sine above this starts slewing into a triangle"};
    inline const ParameterInfo directionality{"directionality", "Slew Bias", ValueType::none, makeRange(-1.0f, 1.0f), 0.0f,
                                              "Slews the rises more than the falls, or the other way round"};
    // waveshape

    inline const ParameterInfo waveshapeDrive{"waveshapeDrive", "Waveshape Drive", ValueType::db, makeRange(-24.0f, 24.0f), 0.0f,
                                              "Gain into the waveshaper, pushing the signal further along the curve"};

    // opto: the audio lighting LEDs, turned down by the LDR they light
    inline const ParameterInfo optoThreshold{"optoThreshold", "Opto Threshold", ValueType::db, makeRange(-48.0f, 0.0f), -18.0f,
                                             "Where the LEDs start lighting the cell, turning the audio down"};
    inline const ParameterInfo optoRatio{"optoRatio", "Opto Ratio", ValueType::compressionRatio, makeRange(1.0f, 20.0f), 4.0f,
                                         "How hard the cell turns the audio down once it's lit past the threshold"};
    inline const ParameterInfo optoCompSpeed{"optoCompSpeed", "Opto Speed", ValueType::ms, makeSkewedRange(0.0f, 100.0f, 0.4f), 5.0f,
                                             "How fast the cell reacts. It lets go 10.5x slower, like an LA-2A"};


                                           
    // dll: debug builds only, whatever the loaded dylib makes of them, see hotdsp/HotDsp.h
    inline const ParameterInfo dllDrive{"dllDrive", "DLL Drive", ValueType::db, makeRange(-24.0f, 24.0f), 0.0f, "DLL drive"};
    inline const ParameterInfo dllA{"dllA", "DLL A", ValueType::none, makeRange(0.0f, 1.0f), 0.5f, "DLL parameter A"};
    inline const ParameterInfo dllB{"dllB", "DLL B", ValueType::none, makeRange(0.0f, 1.0f), 0.5f, "DLL parameter B"};
    inline const ParameterInfo dllC{"dllC", "DLL C", ValueType::none, makeRange(0.0f, 1.0f), 0.5f, "DLL parameter C"};
    inline const ParameterInfo dllD{"dllD", "DLL D", ValueType::none, makeRange(0.0f, 1.0f), 0.5f, "DLL parameter D"};
    inline const ParameterInfo dllE{"dllE", "DLL E", ValueType::none, makeRange(0.0f, 1.0f), 0.5f, "DLL parameter E"};
    inline const ParameterInfo dllF{"dllF", "DLL F", ValueType::none, makeRange(0.0f, 1.0f), 0.5f, "DLL parameter F"};
    inline const ParameterInfo waveshapeX{"waveshapeX", "Waveshape X", ValueType::none, makeRange(0.0f, 1.0f), 0.5f,
                                          "Across the waveshape map, where similar shapes sit together"};
    inline const ParameterInfo waveshapeY{"waveshapeY", "Waveshape Y", ValueType::none, makeRange(0.0f, 1.0f), 0.5f,
                                          "Up the waveshape map, where similar shapes sit together"};
    inline const ParameterInfo waveshapeSmooth{"waveshapeSmooth", "Waveshape Smooth", ValueType::percent, makeRange(0.0f, 100.0f), 0.0f,
                                               "Rounds off the curve's sharp corners without filtering the audio"};
    inline const ParameterInfo waveshapeBias{"waveshapeBias", "Waveshape Bias", ValueType::none, makeRange(-1.0f, 1.0f), 0.0f,
                                             "Offsets into the curve for even harmonics. Silence stays silent"};
    inline const ParameterInfo waveshapeAsym{"waveshapeAsym", "Waveshape Asym", ValueType::percent, makeRange(0.0f, 100.0f), 0.0f,
                                             "Skews the signal: the negative side flattens, the positive speeds up"};

    inline const ParameterInfo slewType{"slewType", "Slew Type", ValueType::category, makeSteppedRange(0.0f, 2.0f), 0.0f,
                                        "A hard limit, one eased into, or one letting small steps through and holding big ones back"};

    // the start page amount that scales a parameter, nullptr for the rest. by id, since slots hold copies of these
    inline const ParameterInfo* scaleFor(const ParameterInfo& info)
    {
        for (const auto* drive : { &saturationAmount, &tubeAmount, &phaseAmount, &rubidiumAmount, &tapeDrive, &alphaParam, &waveshapeDrive })
            if (info.id == drive->id)
                return &globalDrive;

        for (const auto* compression : { &stereoCompThreshold, &MBCompThreshold, &MSCompThreshold, &TypeAThreshold, &TypeARatio, &compRatio, &transientAttack, &transientSustain, &optoThreshold, &optoRatio })
            if (info.id == compression->id)
                return &compAmount;

        for (const auto* emphasis : { &emphasisLowGain, &emphasisHighGain, &emphasisTilt })
            if (info.id == emphasis->id)
                return &eqStrength;

        for (const auto* noise : { &sizzleAmount, &fizzAmount, &erosionAmount, &gateAmt })
            if (info.id == noise->id)
                return &noiseAmount;

        for (const auto* preFx : { &allPassAmount, &grungeAmt, &harmonicAmount, &formantShift, &freqShift })
            if (info.id == preFx->id)
                return &preFxAmount;

        return nullptr;
    }
}

inline juce::ParameterID paramIdFor(SlotId slot, const ParamIDs::ParameterInfo& descriptor)
{
    return { slot.prefix() + "_" + descriptor.id.toString(), descriptor.versionHint };
}
