#pragma once
 
#include "Modules/Module.h"

#include "Modules/Panels/EQPanel.h"
#include "Modules/Panels/TiltPanel.h"
#include "Modules/Panels/ScopePanel.h"
#include "Modules/Panels/ErosionPanel.h"
#include "Modules/Panels/SizzlePanel.h"
#include "Modules/Panels/ReductionPanel.h"
#include "Modules/Panels/GatePanel.h"
#include "Modules/Panels/StartPanels.h"

class UtilColumn : public juce::Component
{
public:
    UtilColumn(AudioPluginAudioProcessor &p)
    {
        // panel with elements already inside
        std::vector<std::unique_ptr<Panel>> eqPanels;
        // ORDERING IS VERY IMPORTANT
        eqPanels.push_back(std::make_unique<EQPanel>(p));
        eqPanels.push_back(std::make_unique<TiltPanel>(p));
        eq = std::make_unique<Module>(p, "EMPHASIS", "emphasisOn", ParamIDs::emphasisType.getParamID().toStdString(), std::move(eqPanels));
        addAndMakeVisible(eq.get());

        std::vector<std::unique_ptr<Panel>> noisePanels;
        // ORDERING IS VERY IMPORTANT
        noisePanels.push_back(std::make_unique<SizzlePanel>(p));
        noisePanels.push_back(std::make_unique<ErosionPanel>(p));
        noisePanels.push_back(std::make_unique<ReductionPanel>(p));
        noisePanels.push_back(std::make_unique<GatePanel>(p));
        noisePanels.push_back(std::make_unique<SizzleOGPanel>(p));

        noise = std::make_unique<Module>(p, "NOISE", SlotId{ModuleId::module1, 0}.enabled().getParamID().toStdString(), SlotId{ModuleId::module1, 0}.type().getParamID().toStdString(), std::move(noisePanels));
        auto noiseLevels = std::make_unique<SlotLevels>(p, SlotId{ModuleId::module1, 0}, ScopeContextType::NOISE);
        noiseLevels->hideNames();
        noise->setFooter(std::move(noiseLevels), SlotLevels::height);
        addAndMakeVisible(noise.get());

        std::vector<std::unique_ptr<Panel>> amountsPanels;
        amountsPanels.push_back(std::make_unique<AmountsPanel>(p));
        amounts = std::make_unique<Module>(p, "", "", "", std::move(amountsPanels), true);
        addChildComponent(amounts.get());

        std::vector<std::unique_ptr<Panel>> settingsPanels;
        settingsPanels.push_back(std::make_unique<ScopePanel>(p));
        settings = std::make_unique<Module>(p, "SETTINGS", "", "", std::move(settingsPanels), true);
        addAndMakeVisible(settings.get());
    }

    ~UtilColumn() {
        eq->setLookAndFeel(nullptr);
        noise->setLookAndFeel(nullptr);
        settings->setLookAndFeel(nullptr);
    }

    // the box for a module this column holds, nullptr for the rest. both emphasis halves are the one box
    Module* moduleFor(ModuleId id) {
        switch (id) {
            case ModuleId::preEmphasis:
            case ModuleId::postEmphasis: return eq.get();
            case ModuleId::module1:      return noise.get();
            default:                     return nullptr;
        }
    }

    static constexpr int startAmountsHeight = 110;

    // the start page has the scope over a row of amounts: compressor, emphasis, noise and pre fx. settings has the scope alone
    enum class Layout { full, start, scopeOnly };

    void setLayout(Layout newLayout) {
        layout = newLayout;
        eq->setVisible(layout == Layout::full);
        noise->setVisible(layout == Layout::full);
        amounts->setVisible(layout == Layout::start);
        resized();
    }

    void resized() override{
        auto bounds = getLocalBounds();

        if (layout != Layout::full)
        {
            if (layout == Layout::start)
                amounts->setBounds(bounds.removeFromBottom(startAmountsHeight));

            settings->setBounds(bounds);
            return;
        }

        const auto row = bounds.getHeight() / 4;

        settings->setBounds(bounds.removeFromTop(row));
        noise->setBounds(bounds.removeFromBottom(row));
        eq->setBounds(bounds);
    }

private:
    std::unique_ptr<Module> eq = nullptr;
    std::unique_ptr<Module> noise = nullptr;
    std::unique_ptr<Module> settings = nullptr;
    std::unique_ptr<Module> amounts = nullptr;

    Layout layout = Layout::full;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UtilColumn)
};