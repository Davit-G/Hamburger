#pragma once

#include <Juceheader.h>

#include "EQPanel.h"

class EmphasisPanel : public juce::Component
{
public:
    EmphasisPanel(AudioPluginAudioProcessor &p) : processorRef(p)
    {
        screen = std::make_unique<EQPanel>(processorRef);
        addAndMakeVisible(screen.get());
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(10);

        if (screen)
            screen->setBounds(bounds);
    }

private:
    AudioPluginAudioProcessor &processorRef;

    std::unique_ptr<juce::Component> screen = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EmphasisPanel);
};