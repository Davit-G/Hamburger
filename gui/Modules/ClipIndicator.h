#pragma once

#include "ScopeDataCollector.h"
#include "../../PluginProcessor.h"
#include "../../dsp/WaveShapers.h"
#include "../../dsp/EffectInfos.h"
#include "../LookAndFeel/Theme.h"

class ClipIndicator : public juce::Component,
                       private juce::Timer
{
public:
    explicit ClipIndicator(ScopeDataCollector<float>& collectorToWatch, AudioPluginAudioProcessor &processor)
        : collector(collectorToWatch), p(processor)
    {
        startTimerHz(20);

        enabledId = clipSlot.enabled().getParamID();
        type = dynamic_cast<juce::AudioParameterChoice*>(p.treeState.getParameter(clipSlot.type().getParamID()));
        kneeId    = paramIdFor(clipSlot, ParamIDs::postClipKnee).getParamID();
    }

    void paint(juce::Graphics& g) override
    {
        // the clipper is bypassed, so there's nothing meaningful to report
        if (*p.treeState.getRawParameterValue(enabledId) < 0.5f)
            return;

        const auto level = collector.clipIndicator.getNext();

        auto dotColour = theme().clipDotIdle;
        if (level >= hardClipLevel)
            dotColour = theme().clipHard;
        // the limiter has no knee, and holds the level right where one would be
        else if (type->getIndex() != limiterType && isSoftClipperKnee(level, *p.treeState.getRawParameterValue(kneeId)))
            dotColour = theme().clipKnee;

        g.setColour(dotColour);
        g.fillEllipse(getLocalBounds().toFloat().reduced(1.0f));
    }

private:
    void timerCallback() override { repaint(); }

    ScopeDataCollector<float>& collector;
    AudioPluginAudioProcessor &p;

    static constexpr float hardClipLevel = 1.0f;
    static constexpr int limiterType = 1; // LIMIT in the clip type menu

    static constexpr SlotId clipSlot { ModuleId::postClip, 0 };
    juce::String enabledId, kneeId;
    juce::AudioParameterChoice* type = nullptr;
};
