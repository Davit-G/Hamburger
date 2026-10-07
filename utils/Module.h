#pragma once

#include "juce_core/juce_core.h"

using PackedOrdering = juce::uint64;

// all the module IDs we will ever need
enum ModuleId : uint8_t
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

using UnpackedOrdering = std::array<ModuleId, static_cast<size_t>(ModuleId::count)>;

static constexpr size_t routingBitsPerSlot = 4;

inline constexpr PackedOrdering packRouting (const UnpackedOrdering& order)
{
    PackedOrdering packed = 0;

    for (size_t i = 0; i < order.size(); ++i)
        packed |= static_cast<PackedOrdering> (order[i]) << (i * routingBitsPerSlot);

    return packed;
}

inline constexpr UnpackedOrdering unpackRouting (PackedOrdering packed)
{
    UnpackedOrdering order {};

    for (size_t i = 0; i < order.size(); ++i)
        order[i] = static_cast<ModuleId> ((packed >> (i * routingBitsPerSlot)) & 0xf);

    return order;
}



// module info just matches enum ids to names and param ids and other stuff like that
struct ModuleInfo
{
    ModuleId id;
    const char *key;
    const char *displayName;
    int slotCount = 1; // set to 0 to ignore generating macros, and hand roll the params yourself. set to 1 or higher for macros + slots
    bool enabledByDefault = true; // initial state of the slot's generated on/off toggle. for example allpass is disabled but distortion is enabled
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

// if we are missing a module ID this will catch it. should be the same number of IDs as entries here
static_assert (std::size (pluginModules) == static_cast<size_t> (ModuleId::count));

// on compile time, generate the default routing we wanna use
static constexpr PackedOrdering defaultOrdering = [](){
    UnpackedOrdering chain {};

    for (size_t i = 0; i < std::size (pluginModules); ++i)
        chain[i] = pluginModules[i].id;

    return packRouting(chain);
}();