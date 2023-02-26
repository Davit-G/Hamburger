#pragma once
#include <JuceHeader.h>

#include "Classic.h"
#include "Tube.h"

#include "../LookAndFeel/ComboBoxLookAndFeel.h"

#include "../LightButton.h"

enum class DistortionsPages
{
    Classic = 0,
    Tube
};


class DistortionsPanel : public juce::Component {
public:
    DistortionsPanel(AudioPluginAudioProcessor &p) : processorRef(p)  {

        enableDistortionButton = std::make_unique<LightButton>(p, "primaryDistortionEnabled", powerOffImage, powerOnImage);
        addAndMakeVisible(enableDistortionButton.get());
        
        setScreen(DistortionsPages::Classic);

        distortionSelectorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.treeState, "primaryDistortionType", preDistortionSelector);
        preDistortionSelector.setLookAndFeel(&comboBoxLook);
        preDistortionSelector.addItemList(StringArray({ "Classic", "Tube" }), 1);
        preDistortionSelector.setSelectedItemIndex(0);
        preDistortionSelector.toFront(false);
        addAndMakeVisible(&preDistortionSelector);

        preDistortionSelector.onChange = [this] {
            auto selection = preDistortionSelector.getSelectedItemIndex();

            if (selection != -1) {
                setScreen(static_cast<DistortionsPages>(selection));
            }
        };


    };

    ~DistortionsPanel(){};

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

        enableDistortionButton->setBounds(7, 5, 18, 18);

    }

     void setScreen(DistortionsPages type)
    {
        std::unique_ptr<juce::Component> newScreen;
        switch (type)
        {
        case DistortionsPages::Classic:
            newScreen = std::make_unique<Classic>(processorRef);
            break;
        case DistortionsPages::Tube:
            newScreen = nullptr; //std::make_unique<Tube>(processorRef);
            break;
        // case DistortionsPages::Comb:
        //     newScreen = nullptr; // std::make_unique<EffectsScreen>();
        //     break;
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
    std::unique_ptr<LightButton> enableDistortionButton;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> distortionSelectorAttachment;

    std::unique_ptr<juce::Component> screen = nullptr;


    juce::Image powerOffImage = juce::ImageCache::getFromMemory(BinaryData::poweroff_png, BinaryData::poweroff_pngSize);
    juce::Image powerOnImage = juce::ImageCache::getFromMemory(BinaryData::poweron_png, BinaryData::poweron_pngSize);
};