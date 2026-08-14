#pragma once
#include "../SmoothParam.h"

#if PERFETTO
#include <melatonin_perfetto/melatonin_perfetto.h>
#endif // PERFETTO
#include "../../utils/Params.h"

#include "../../gui/Modules/ScopeDataCollector.h"

class PostClip
{
public:
    PostClip(juce::AudioProcessorValueTreeState& treeState, ScopeDataCollector<float>& scopeDataCollector);

    ~PostClip();

    void processBlock(juce::dsp::AudioBlock<float>& block);
    void prepare(juce::dsp::ProcessSpec& spec);

private:
    SmoothParam gainKnob;
    SmoothParam kneeKnob;

    juce::AudioParameterBool *clipEnabled;

    ScopeDataCollector<float>& scopeData;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PostClip)
};