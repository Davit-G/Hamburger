#pragma once 

#include "Modules/Module.h"

#include "Modules/Panels/ClassicSatPanel.h"
#include "Modules/Panels/TubeSatPanel.h"
#include "Modules/Panels/TapeSatPanel.h"
#include "Modules/Panels/RubidiumSatPanel.h" 
#include "Modules/Panels/PhaseDistPanel.h"
#include "Modules/Panels/PostClipPanel.h"
#include "Modules/Panels/ErosionPanel.h"
#include "Modules/Panels/SizzlePanel.h"
#include "Modules/Panels/ReductionPanel.h"
#include "Modules/Panels/GatePanel.h"
#include "Modules/Panels/SlewSaturator.h"

#include "LookAndFeel/Palette.h"

#include "Modules/ClipIndicator.h"
#include "HeaderTabs.h"

class SaturationColumn : public juce::Component
{
public:
    SaturationColumn(AudioPluginAudioProcessor &p) : scopeContext(p.getScopeContext()), clipDot(p.getScopeDataCollector(), p) {
        setInterceptsMouseClicks(true, true);

        // the same distortion box for each tab, only the selected one is shown
        for (size_t i = 0; i < distortions.size(); ++i) {
            const auto [module, title] = distortionSlots[i];
            const SlotId slot { module, 0 };

            // ORDERING IS VERY IMPORTANT, it has to match the order of the slot's type choices
            std::vector<std::unique_ptr<Panel>> panels;
            panels.push_back(std::make_unique<ClassicSatPanel>(p, slot));
            panels.push_back(std::make_unique<TubeSatPanel>(p, slot));
            panels.push_back(std::make_unique<PhaseDistPanel>(p, slot));
            panels.push_back(std::make_unique<RubidiumSatPanel>(p, slot));
            panels.push_back(std::make_unique<TapeSatPanel>(p, slot));
            panels.push_back(std::make_unique<SlewRatePanel>(p, slot));

            distortions[i] = std::make_unique<Module>(p, title, slot.enabled().getParamID().toStdString(), slot.type().getParamID().toStdString(), std::move(panels));
            addChildComponent(distortions[i].get());
        }

        setView(HeaderTabs::main);

        std::vector<std::unique_ptr<Panel>> clipPanel;

        postClipPanel = std::make_unique<PostClipPanel>(p);
        clipPanel.push_back(std::move(postClipPanel));

        postClip = std::make_unique<Module>(p, "CLIPPER", SlotId{ModuleId::postClip, 0}.enabled().getParamID().toStdString(), "", std::move(clipPanel));
        addAndMakeVisible(postClip.get());

        std::vector<std::unique_ptr<Panel>> noisePanels;
        // ORDERING IS VERY IMPORTANT
        noisePanels.push_back(std::make_unique<SizzlePanel>(p));
        noisePanels.push_back(std::make_unique<ErosionPanel>(p));
        noisePanels.push_back(std::make_unique<ReductionPanel>(p));
        noisePanels.push_back(std::make_unique<GatePanel>(p));
        noisePanels.push_back(std::make_unique<SizzleOGPanel>(p));

        noise = std::make_unique<Module>(p, "NOISE", SlotId{ModuleId::module1, 0}.enabled().getParamID().toStdString(), SlotId{ModuleId::module1, 0}.type().getParamID().toStdString(), std::move(noisePanels));
        addAndMakeVisible(noise.get());

        addAndMakeVisible(clipDot);
    }

    // void mouseUp(const juce::MouseEvent &event) override {
    //     scopeContext.setType(ScopeContextType::IN_OUT);
    // }

    // void mouseDown(const juce::MouseEvent &event) override {
    //     scopeContext.setType(ScopeContextType::LR_SCOPE);
    // }

    void setView(HeaderTabs::Tab tab) {
        scopeContext.setFocus({ distortionSlots[tab].first, 0 });

        for (size_t i = 0; i < distortions.size(); ++i)
            distortions[i]->setVisible(i == (size_t) tab);
    }

    Module& getDistortion(HeaderTabs::Tab tab) { return *distortions[tab]; }

    // the box on screen for a module this column holds, nullptr for the rest
    Module* moduleFor(ModuleId id) {
        for (size_t i = 0; i < distortions.size(); ++i)
            if (distortionSlots[i].first == id)
                return distortions[i].get();

        return id == ModuleId::module1 ? noise.get() : id == ModuleId::postClip ? postClip.get() : nullptr;
    }

    ~SaturationColumn() {
        for (auto& distortion : distortions)
            distortion->setLookAndFeel(nullptr);
        postClip->setLookAndFeel(nullptr);
        noise->setLookAndFeel(nullptr);
    }

    void resized() override{
        auto bounds = getLocalBounds();
        auto height = bounds.getHeight();

        const auto top = bounds.removeFromTop(height * 3/4);
        for (auto& distortion : distortions)
            distortion->setBounds(top);
        
        auto postClipBounds = bounds.removeFromRight(bounds.getWidth() / 2);
        postClip->setBounds(postClipBounds);
        clipDot.setBounds(postClipBounds.removeFromTop(dotSize).removeFromRight(dotSize).reduced(4).translated(-19, 19));
        
        noise->setBounds(bounds);
    }

private:
    ScopeContext& scopeContext;

    // in the order of HeaderTabs' PRE | MAIN | POST
    static constexpr std::pair<ModuleId, const char*> distortionSlots[] {
        { ModuleId::preDistortion, "PRE-DISTORTION" },
        { ModuleId::main, "DISTORTION" },
        { ModuleId::postDistortion, "POST-DISTORTION" },
    };

    std::array<std::unique_ptr<Module>, std::size(distortionSlots)> distortions;

    std::unique_ptr<Panel> postClipPanel = nullptr;

    std::unique_ptr<Module> noise = nullptr;
    std::unique_ptr<Module> postClip = nullptr;

    static constexpr int dotSize = 16;
    ClipIndicator clipDot;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SaturationColumn)
};