#pragma once

#include <Juceheader.h>

#include "EQPanel.h"
#include "ShiftPanel.h"
#include "../LookAndFeel/ComboBoxLookAndFeel.h"

enum class EmphasisPages
{
    EQ = 0,
    Shift,
    RM
};

class EmphasisPanel : public juce::Component
{
public:
    EmphasisPanel(AudioPluginAudioProcessor &p) : processorRef(p)
    {
        setScreen(EmphasisPages::EQ);
        optionSelector.setLookAndFeel(&comboBoxLook);
        optionSelector.addItemList(StringArray({"EQ", "Shift", "RM"}), 1);
        optionSelector.setSelectedItemIndex(0);

        optionSelector.onChange = [this] {
            auto selection = optionSelector.getSelectedItemIndex();

            if (selection != -1) {
                setScreen(static_cast<EmphasisPages>(selection));
            }
        };

        addAndMakeVisible(optionSelector);
    }

    ~EmphasisPanel() {}

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto buttons = bounds.removeFromTop(30);

        if (screen)
            screen->setBounds(bounds);
        
        optionSelector.setBounds(buttons);
    }

    void setScreen(EmphasisPages type)
    {
        std::unique_ptr<juce::Component> newScreen;
        switch (type)
        {
        case EmphasisPages::EQ:
            newScreen = std::make_unique<EQPanel>(processorRef);
            break;
        case EmphasisPages::Shift:
            newScreen =  std::make_unique<ShiftPanel>(processorRef);
            break;
        case EmphasisPages::RM:
            newScreen = nullptr;// std::make_unique<EffectsScreen>();
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
    AudioPluginAudioProcessor &processorRef;

    std::unique_ptr<juce::Component> screen = nullptr;

    ComboBox optionSelector;

    ComboBoxLookAndFeel comboBoxLook;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EmphasisPanel);
};