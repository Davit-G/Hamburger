#pragma once

#include <Juceheader.h>

#include "EQPanel.h"

enum class EmphasisPages
{
    EQ = 0,
    Shift,
    RM
};

class EmphasisPanel : public juce::Component
{
public:
    EmphasisPanel()
    {
        setScreen(EmphasisPages::EQ);
    }

    ~EmphasisPanel() {}

    void resized() override
    {
        if (screen) screen->setBounds(getLocalBounds());
    }

    void setScreen(EmphasisPages type)
    {
        std::unique_ptr<juce::Component> newScreen;
        switch (type)
        {
        case EmphasisPages::EQ:
            // newScreen = std::make_unique<StartScreen>();
            break;
        case EmphasisPages::Shift:
            // newScreen = std::make_unique<SequencerScreen>();
            break;
        case EmphasisPages::RM:
            // newScreen = std::make_unique<EffectsScreen>();
            break;
        default:
            break;
        }
        if (newScreen)
        {
            addAndMakeVisible(newScreen.get());
            screen = std::move(newScreen);
            resized();
        }
        else
            screen.reset();
    }

private:
    std::unique_ptr<juce::Component> screen = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EmphasisPanel);
};