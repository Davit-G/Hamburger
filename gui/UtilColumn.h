#pragma once

#include <JuceHeader.h>
#include "Modules/Module.h"

#include "Modules/Panels/EQPanel.h"
#include "Modules/Panels/UtilityPanel.h"


class UtilColumn : public juce::Component
{
public:
    UtilColumn(AudioPluginAudioProcessor &p)
    {
        // panel with elements already inside
        std::vector<std::unique_ptr<Panel>> eqPanels;
        // ORDERING IS VERY IMPORTANT
        eqPanels.push_back(std::make_unique<EQPanel>(p));
        eq = std::make_unique<Module>(p, "EMPHASIS", "emphasisOn", "", std::move(eqPanels));
        addAndMakeVisible(eq.get());

        std::vector<std::unique_ptr<Panel>> utilityPanels;
        // ORDERING IS VERY IMPORTANT
        utilityPanels.push_back(std::make_unique<UtilityPanel>(p));
        utility = std::make_unique<Module>(p, "UTILITY", "hamburgerEnabled", "", std::move(utilityPanels));
        addAndMakeVisible(utility.get());
    }

    ~UtilColumn() override{}

    void resized() override{
        auto bounds = getLocalBounds();
        auto height = bounds.getHeight();

        eq->setBounds(bounds.removeFromTop(height / 2.5f));
        utility->setBounds(bounds.removeFromBottom(height / 3));
    }

private:
    // AudioPluginAudioProcessor &processorRef;

    std::unique_ptr<Module> eq;
    std::unique_ptr<Module> utility;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UtilColumn)
};