#pragma once
#include <JuceHeader.h>

#include "DisperserPanel.h"
#include "ReverbPanel.h"
#include "CombPanel.h"

#include "../LookAndFeel/ComboBoxLookAndFeel.h"

#include "../LightButton.h"

enum class PreDistortionsPages
{
    AllPass = 0,
    Reverb,
    Comb
};


class PreDistortions : public juce::Component {
public:
    PreDistortions(AudioPluginAudioProcessor &p) : processorRef(p)  {

        enablePreDistortionButton = std::make_unique<LightButton>(p, "preDistortionEnabled", powerOffImage, powerOnImage);
        addAndMakeVisible(enablePreDistortionButton.get());
        
        setScreen(PreDistortionsPages::AllPass);

        preDistortionSelectorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.treeState, "preDistortionType", preDistortionSelector);
        preDistortionSelector.setLookAndFeel(&comboBoxLook);
        preDistortionSelector.addItemList(StringArray({ "Disperser", "Reverb", "Comb" }), 1);
        preDistortionSelector.setSelectedItemIndex(0);
        preDistortionSelector.toFront(false);
        addAndMakeVisible(&preDistortionSelector);

        preDistortionSelector.onChange = [this] {
            auto selection = preDistortionSelector.getSelectedItemIndex();

            if (selection != -1) {
                setScreen(static_cast<PreDistortionsPages>(selection));
            }
        };


    };

    ~PreDistortions(){};

    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(20);

        if (screen)
            screen->setBounds(bounds);
            
        auto dims = getLocalBounds().removeFromTop(30);
        dims.setWidth(200);
        dims.setX(30);
        preDistortionSelector.setBounds(dims);

        enablePreDistortionButton->setBounds(7, 5, 18, 18);

    }

     void setScreen(PreDistortionsPages type)
    {
        std::unique_ptr<juce::Component> newScreen;
        switch (type)
        {
        case PreDistortionsPages::AllPass:
            newScreen = std::make_unique<DisperserPanel>(processorRef);
            break;
        case PreDistortionsPages::Reverb:
            newScreen = std::make_unique<ReverbPanel>(processorRef);
            break;
        case PreDistortionsPages::Comb:
            newScreen = std::make_unique<CombPanel>(processorRef);
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

    ComboBox preDistortionSelector;
    ComboBoxLookAndFeel comboBoxLook;
    std::unique_ptr<LightButton> enablePreDistortionButton;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> preDistortionSelectorAttachment;

    std::unique_ptr<juce::Component> screen = nullptr;


    juce::Image powerOffImage = juce::ImageCache::getFromMemory(BinaryData::poweroff_png, BinaryData::poweroff_pngSize);
    juce::Image powerOnImage = juce::ImageCache::getFromMemory(BinaryData::poweron_png, BinaryData::poweron_pngSize);
};