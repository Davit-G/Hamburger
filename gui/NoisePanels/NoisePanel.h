#pragma once
#include <JuceHeader.h>

#include "ErosionPanel.h"
#include "SizzlePanel.h"
#include "ReduxPanel.h"
#include "JeffPanel.h"

#include "../LookAndFeel/ComboBoxLookAndFeel.h"

#include "../LightButton.h"

enum class NoisePages
{
    Sizzle = 0,
    Erosion,
    Asperity,
    Redux,
    Jeff
};


class NoisePanel : public juce::Component {
public:
    NoisePanel(AudioPluginAudioProcessor &p) : processorRef(p)  {

        enableNoiseButton = std::make_unique<LightButton>(p, "noiseDistortionEnabled", powerOffImage, powerOnImage);
        addAndMakeVisible(enableNoiseButton.get());
        
        setScreen(NoisePages::Sizzle);

        noiseSelectorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.treeState, "noiseDistortionType", noiseSelector);
        noiseSelector.setLookAndFeel(&comboBoxLook);
        noiseSelector.addItemList(StringArray({ "Sizzle", "Erosion", "Asperity", "Downsample / Bitreduction", "Jeff Thickness"}), 1);
        noiseSelector.setSelectedItemIndex(0);
        noiseSelector.toFront(false);
        addAndMakeVisible(&noiseSelector);

        noiseSelector.onChange = [this] {
            auto selection = noiseSelector.getSelectedItemIndex();

            if (selection != -1) {
                setScreen(static_cast<NoisePages>(selection));
            }
        };

    };

    ~NoisePanel(){};

    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(20);

        if (screen)
            screen->setBounds(bounds);
            
        auto dims = getLocalBounds().removeFromTop(30);
        dims.setWidth(200);
        dims.setX(30);
        noiseSelector.setBounds(dims);

        enableNoiseButton->setBounds(7, 5, 18, 18);

    }

     void setScreen(NoisePages type)
    {
        std::unique_ptr<juce::Component> newScreen;
        switch (type)
        {
        case NoisePages::Sizzle:
            newScreen = std::make_unique<SizzlePanel>(processorRef);
            break;
        case NoisePages::Erosion:
            newScreen = std::make_unique<ErosionPanel>(processorRef);
            break;
        case NoisePages::Asperity:
            newScreen = nullptr;
            break;
        case NoisePages::Redux:
            newScreen = std::make_unique<ReduxPanel>(processorRef);
            break;
        case NoisePages::Jeff:
            newScreen = std::make_unique<JeffPanel>(processorRef);
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

    ComboBox noiseSelector;
    ComboBoxLookAndFeel comboBoxLook;
    std::unique_ptr<LightButton> enableNoiseButton;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> noiseSelectorAttachment;

    std::unique_ptr<juce::Component> screen = nullptr;


    juce::Image powerOffImage = juce::ImageCache::getFromMemory(BinaryData::poweroff_png, BinaryData::poweroff_pngSize);
    juce::Image powerOnImage = juce::ImageCache::getFromMemory(BinaryData::poweron_png, BinaryData::poweron_pngSize);
};