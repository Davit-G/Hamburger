#pragma once

#include "../Panel.h"
#include "PanelNames.h"

class SettingsPanel : public Panel
{
public:
    SettingsPanel(AudioPluginAudioProcessor &p) : Panel(p, "SETTINGS"), 
    qualityOption("QUALITY")
    {
        qualityOption.addItem("LOW", 1);
        qualityOption.addItem("MEDIUM", 2);
        qualityOption.addItem("HIGH", 3);
        qualityOption.setSelectedId(2);
        setLookAndFeel(new ComboBoxLookAndFeel());
        addAndMakeVisible(qualityOption);

        qualityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.treeState, "qualityFactor", qualityOption);
    }

    void resized() override {
        // three, in a row
        auto bounds = getLocalBounds();
        qualityOption.setBounds(bounds);
    }

private:

    ComboBox qualityOption;
    // attachment for quality option
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> qualityAttachment = nullptr;

};