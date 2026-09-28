#pragma once

 
#include "Modules/Module.h"
#include "Modules/Panels/MBCompPanel.h"
#include "Modules/Panels/MSCompPanel.h"
#include "Modules/Panels/StereoCompPanel.h"
#include "Modules/Panels/EmptyPanel.h"
#include "Modules/Panels/AllPassPanel.h"
#include "Modules/Panels/GrungePanel.h"
#include "Modules/Panels/SubGenPanel.h"
#include "Modules/Panels/LogoPanel.h"
#include "Modules/Panels/TypeAPanel.h"
#include "Modules/Panels/TransientPanel.h"
#include "Modules/Panels/OptoPanel.h"

class LeftColumn : public juce::Component
{
public:
    LeftColumn(AudioPluginAudioProcessor &p)
    {
        // panel with elements already inside
        std::vector<std::unique_ptr<Panel>> companderPanels;
        // ORDERING IS VERY IMPORTANT
        
        companderPanels.push_back(std::make_unique<StereoCompPanel>(p));
        companderPanels.push_back(std::make_unique<MBCompPanel>(p));
        companderPanels.push_back(std::make_unique<MSCompPanel>(p));
        companderPanels.push_back(std::make_unique<TypeAPanel>(p));
        companderPanels.push_back(std::make_unique<TransientPanel>(p, false));
        companderPanels.push_back(std::make_unique<TransientPanel>(p, true));
        companderPanels.push_back(std::make_unique<OptoCompPanel>(p, false));
        companderPanels.push_back(std::make_unique<OptoCompPanel>(p, true));
        compander = std::make_unique<Module>(p, "COMP", SlotId{ModuleId::dynamics, 0}.enabled().getParamID().toStdString(), SlotId{ModuleId::dynamics, 0}.type().getParamID().toStdString(), std::move(companderPanels));

        // by what they work across, by their index in the type menu
        const auto& comp = ParamIDs::compTypes.categories;
        compander->categorySelector.groups = {
            { "SINGLE BAND", { { 0, comp[0] }, { 2, comp[2] }, { 6, comp[6] } } },
            { "MULTIBAND", { { 1, comp[1] }, { 3, comp[3] }, { 7, comp[7] } } },
            { "TRANSIENT", { { 4, comp[4] }, { 5, comp[5] } } },
        };
        auto companderLevels = std::make_unique<SlotLevels>(p, SlotId{ModuleId::dynamics, 0}, ScopeContextType::COMPRESSION);
        companderLevels->hideNames();
        compander->setFooter(std::move(companderLevels), SlotLevels::height);
        addAndMakeVisible(compander.get());
        

        std::vector<std::unique_ptr<Panel>> preDistortionPanels;
        // ORDERING IS VERY IMPORTANT
        preDistortionPanels.push_back(std::make_unique<AllPassPanel>(p));
        preDistortionPanels.push_back(std::make_unique<GrungePanel>(p));
        preDistortionPanels.push_back(std::make_unique<SubGenPanel>(p));
        preDistortionPanels.push_back(std::make_unique<HilbertStackPanel>(p));
        preDistortion = std::make_unique<Module>(p, "", SlotId{ModuleId::module2, 0}.enabled().getParamID().toStdString(), SlotId{ModuleId::module2, 0}.type().getParamID().toStdString(), std::move(preDistortionPanels));
        auto preDistortionLevels = std::make_unique<SlotLevels>(p, SlotId{ModuleId::module2, 0}, ScopeContextType::LR_SCOPE);
        preDistortionLevels->hideNames();
        preDistortion->setFooter(std::move(preDistortionLevels), SlotLevels::height);
        addAndMakeVisible(preDistortion.get());

        std::vector<std::unique_ptr<Panel>> logoPanels;
        logoPanels.push_back(std::make_unique<LogoPanel>(p));

        logo = std::make_unique<Module>(p, "CREDITS", "", "", std::move(logoPanels), true);
        addAndMakeVisible(logo.get());

    }

    ~LeftColumn() {
        compander->setLookAndFeel(nullptr);
        preDistortion->setLookAndFeel(nullptr);
        logo->setLookAndFeel(nullptr);
    }

    // the box for a module this column holds, nullptr for the rest
    Module* moduleFor(ModuleId id) {
        switch (id) {
            case ModuleId::dynamics: return compander.get();
            case ModuleId::module2:  return preDistortion.get();
            default:                 return nullptr;
        }
    }

    void paint(juce::Graphics &g) override
    {
        juce::ignoreUnused(g);
    }

    // settings keeps the logo, and the tooltips it shows, on its own
    void setLogoOnly(bool shouldShowLogoOnly) {
        logoOnly = shouldShowLogoOnly;
        compander->setVisible(! logoOnly);
        resized();
    }

    void resized() override{
        auto bounds = getLocalBounds();

        if (logoOnly)
            return logo->setBounds(bounds);

        logo->setBounds(bounds.removeFromTop(bounds.getHeight() / 3));
        compander->setBounds(bounds);
    }

    // the pre fx box, which the editor lays out along the bottom with the noise, under the modulation
    Module& getPreFx() { return *preDistortion; }

private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableButtonAttachment = nullptr;

    std::unique_ptr<Module> compander = nullptr;
    std::unique_ptr<Module> preDistortion = nullptr;
    std::unique_ptr<Module> logo = nullptr;
    std::unique_ptr<Module> typeA = nullptr;

    bool logoOnly = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LeftColumn)
};