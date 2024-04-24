#pragma once
 
#include "Modules/Module.h"

#include "Modules/Panels/EQPanel.h"
#include "Modules/Panels/UtilityPanel.h"
#include "Modules/Panels/ScopePanel.h"
#include "Modules/Panels/OtherUtils.h"
#include "LookAndFeel/Palette.h"

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
        eq->setLookAndFeel(&eqLookAndFeel);
        addAndMakeVisible(eq.get());

        std::vector<std::unique_ptr<Panel>> utilityPanels;
        // ORDERING IS VERY IMPORTANT
        utilityPanels.push_back(std::make_unique<UtilityPanel>(p));
        utilityPanels.push_back(std::make_unique<OtherUtils>(p));
        utility = std::make_unique<Module>(p, "", "hamburgerEnabled", "", std::move(utilityPanels));
        utility->setLookAndFeel(&utilityLookAndFeel);
        addAndMakeVisible(utility.get());

        std::vector<std::unique_ptr<Panel>> settingsPanels;
        settingsPanels.push_back(std::make_unique<ScopePanel>(p));
        settings = std::make_unique<Module>(p, "SETTINGS", "", "", std::move(settingsPanels), true);
        addAndMakeVisible(settings.get());
    }

    ~UtilColumn() {
        eq->setLookAndFeel(nullptr);
        utility->setLookAndFeel(nullptr);
        settings->setLookAndFeel(nullptr);
    }

    void resized() override{
        auto bounds = getLocalBounds();
        auto height = bounds.getHeight();

        settings->setBounds(bounds.removeFromTop(height / 4));
        utility->setBounds(bounds.removeFromBottom(height / 4));
        eq->setBounds(bounds);
    }

private:
    KnobLAF eqLookAndFeel = KnobLAF(Palette::colours[2]);
    KnobLAF utilityLookAndFeel = KnobLAF(juce::Colours::whitesmoke);

    std::unique_ptr<Module> eq = nullptr;
    std::unique_ptr<Module> utility = nullptr;
    std::unique_ptr<Module> settings = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UtilColumn)
};