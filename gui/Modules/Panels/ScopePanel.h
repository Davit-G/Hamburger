#pragma once

#include "../Panel.h"


#include "../Scope.h"

class ScopePanel : public Panel
{
public:
    ScopePanel(AudioPluginAudioProcessor &p) : Panel(p, "SETTINGS"),
                                                  // qualityOption("QUALITY")
                                                  scope(p.getAudioBufferQueueL(), p.getAudioBufferQueueR())
    {
        // qualityOption.addItem("LOW", 1);
        // qualityOption.addItem("MEDIUM", 2);
        // qualityOption.addItem("HIGH", 3);
        // qualityOption.setSelectedId(2);
        // setLookAndFeel(new ComboBoxLookAndFeel());
        // addAndMakeVisible(qualityOption);

        addAndMakeVisible (scope);

        // qualityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.treeState, "qualityFactor", qualityOption);
    }

    void resized() override
    {
        // three, in a row
        auto bounds = getLocalBounds();
        scope.setBounds(bounds);
        // qualityOption.setBounds(bounds);
    }

private:
    // ComboBox qualityOption;

    Scope<float> scope;
    // attachment for quality option
    // std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> qualityAttachment = nullptr;
};