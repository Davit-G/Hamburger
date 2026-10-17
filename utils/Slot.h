#pragma once

#include "Module.h"
#include "juce_audio_processors/juce_audio_processors.h"

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

    juce::ParameterID type() const { return { prefix() + "_type",    versionHint() }; }
    juce::ParameterID enabled() const { return { prefix() + "_enabled", versionHint() }; }
};
