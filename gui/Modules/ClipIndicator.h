#pragma once

#include "ScopeDataCollector.h"

#include "../../dsp/WaveShapers.h"

class ClipIndicator : public juce::Component,
                       private juce::Timer
{
public:
    explicit ClipIndicator(ScopeDataCollector<float>& collectorToWatch, AudioPluginAudioProcessor &processor)
        : collector(collectorToWatch), p(processor)
    {
        startTimerHz(20);
    }

    void paint(juce::Graphics& g) override
    {
        const auto level = collector.clipLevel.load(std::memory_order_relaxed);

        auto dotColour = juce::Colours::darkgrey;
        if (level >= hardClipLevel)
            dotColour = juce::Colours::red;
        else if (isSoftClipperKnee(level, 1.0f, *p.treeState.getRawParameterValue(ParamIDs::postClipKnee.getParamID())))
            dotColour = juce::Colours::orange;

        g.setColour(dotColour);
        g.fillEllipse(getLocalBounds().toFloat().reduced(1.0f));
    }

private:
    void timerCallback() override { repaint(); }

    ScopeDataCollector<float>& collector;
    AudioPluginAudioProcessor &p;

    static constexpr float hardClipLevel = 1.0f;
};
