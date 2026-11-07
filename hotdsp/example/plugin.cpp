// the plugin's own .cpp files this dsp uses, compiled in here as a file of their own so editing initdsp.cpp doesn't
// rebuild them. only ones that need no more juce than juce_dsp, anything including Params.h won't build here
#include "../dsp/EnvelopeFollower.cpp"
