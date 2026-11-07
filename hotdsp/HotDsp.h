#pragma once

/*  Everything the plugin's debug only DLL distortion and a hot reloaded dylib share. No juce, so a dylib only pulls in
    what it wants itself. Build one with HOTDSP_EXPORT (YourClass) in a .cpp, pick it in the DLL box, and rebuild: the
    plugin sees the file change, makes a new instance, prepares it, swaps it in and destroys the old one. */

// what the plugin knows about each parameter it sends. the name points into the plugin, good until the next prepare
struct HotDspParam
{
    const char* name;
    float min, max, defaultValue;
};

struct HotDspSpec
{
    double sampleRate;  // oversampled already, it's the rate process runs at
    int numChannels;
    int maxSamples;     // no process call is longer, most are shorter
    const HotDspParam* params;
    int numParams;
};

class HotDsp
{
public:
    virtual ~HotDsp() = default;

    // called before the first process and again whenever the rate, channels or block size change
    virtual void prepare (const HotDspSpec& spec) = 0;

    /*  channels[ch][n] in place, numSamples of them, at most spec.maxSamples. params[p][n] is parameter p at sample n,
        smoothed and modulated, in its own range (see spec.params). numParams can grow, so read what you need */
    virtual void process (float* const* channels, int numChannels, int numSamples,
                          const float* const* params, int numParams) = 0;
};

// the one symbol the plugin looks for. the instance is deleted through its virtual destructor, so it frees in the dylib
using HotDspCreate = HotDsp* (*)();

#define HOTDSP_EXPORT(Class) \
    extern "C" __attribute__ ((visibility ("default"))) HotDsp* hotdspCreate() { return new Class(); }
