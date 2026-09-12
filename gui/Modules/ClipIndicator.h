#pragma once

#include "ScopeDataCollector.h"
#include "../../PluginProcessor.h"
#include "../../dsp/WaveShapers.h"
#include "../../dsp/EffectInfos.h"

class ClipIndicator : public juce::Component,
                       private juce::Timer
{
public:
    explicit ClipIndicator(ScopeDataCollector<float>& collectorToWatch, AudioPluginAudioProcessor &processor)
        : collector(collectorToWatch), p(processor)
    {
        startTimerHz(20);

        enabledId = clipSlot.enabled().getParamID();
        kneeId    = paramIdFor(clipSlot, ParamIDs::postClipKnee).getParamID();
    }

    void paint(juce::Graphics& g) override
    {
        // the clipper is bypassed, so there's nothing meaningful to report
        if (*p.treeState.getRawParameterValue(enabledId) < 0.5f)
            return;

        const auto level = collector.clipIndicator.getNext();

        auto dotColour = juce::Colours::darkgrey;
        if (level >= hardClipLevel)
            dotColour = juce::Colours::red;
        else if (isSoftClipperKnee(level, 1.0f, *p.treeState.getRawParameterValue(kneeId)))
            dotColour = juce::Colours::orange;

        g.setColour(dotColour);
        g.fillEllipse(getLocalBounds().toFloat().reduced(1.0f));
    }

private:
    void timerCallback() override { repaint(); }

    ScopeDataCollector<float>& collector;
    AudioPluginAudioProcessor &p;

    static constexpr float hardClipLevel = 1.0f;

    static constexpr SlotId clipSlot { ModuleId::postClip, 0 };
    juce::String enabledId, kneeId;
};
