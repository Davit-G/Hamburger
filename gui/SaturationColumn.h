#pragma once 

#include "Modules/Module.h"

#include "Modules/DistortionModule.h"
#include "PluginHeader.h"
#include "Modules/Panels/SlewSaturator.h"

#include "Modules/Panels/RoutingPanel.h"
#include "Modules/Panels/StartPanels.h"
#include "ModulationPanels.h"

class SaturationColumn : public juce::Component
{
public:
    SaturationColumn(AudioPluginAudioProcessor &p) : scopeContext(p.getScopeContext()), routingPanel(p), modulationRouting(p) {
        setInterceptsMouseClicks(true, true);

        for (int i = 0; i < MainRouting::maxSlots; ++i)
        {
            mainModules[(size_t) i] = makeDistortionModule(p, SlotId{ModuleId::main, i}, "DISTORTION");
            mainModules[(size_t) i]->onScreenChanged = [this] { updateRoutingAccent(); };
            addChildComponent(mainModules[(size_t) i].get());
        }

        preModule = makeDistortionModule(p, SlotId{ModuleId::preDistortion, 0}, "PRE-DISTORTION");
        addChildComponent(preModule.get());

        postModule = makeDistortionModule(p, SlotId{ModuleId::postDistortion, 0}, "POST-DISTORTION");
        addChildComponent(postModule.get());

        std::vector<std::unique_ptr<Panel>> startPanels;
        startPanels.push_back(std::make_unique<StartPanel>(p));
        startModule = std::make_unique<Module>(p, "DISTORTION", "", "", std::move(startPanels));
        addChildComponent(startModule.get());

        addChildComponent(modulationRouting);

        routingPanel.onSlotSelected = [this](int slot) { setActiveSlot(slot); };

        routingPanel.onRoutingChanged = [this] {
            if (view == PluginHeader::Tab::main)
                setView(view);
        };

        setActiveSlot(0);
        setView(PluginHeader::Tab::main);
    }

    // void mouseUp(const juce::MouseEvent &event) override {
    //     scopeContext.setType(ScopeContextType::IN_OUT);
    // }

    // void mouseDown(const juce::MouseEvent &event) override {
    //     scopeContext.setType(ScopeContextType::LR_SCOPE);
    // }

    // the box on screen for a module this column holds, nullptr for the rest
    Module* moduleFor(ModuleId id) {
        switch (id) {
            case ModuleId::main:           return mainModules[(size_t) activeSlot].get();
            case ModuleId::preDistortion:  return preModule.get();
            case ModuleId::postDistortion: return postModule.get();
            default:                       return nullptr;
        }
    }

    // the 1-4 keys: only on the main view, and by position on screen rather than slot number
    bool selectPosition(int position) {
        return view == PluginHeader::Tab::main && routingPanel.selectPosition(position);
    }

    void setActiveSlot(int slot) {
        activeSlot = juce::jlimit(0, MainRouting::maxSlots - 1, slot);

        if (view == PluginHeader::Tab::main)
            setView(view);
    }

    void setView(PluginHeader::Tab tab) {
        view = tab;

        if (tab == PluginHeader::Tab::pre)
            scopeContext.setFocus(SlotId{ModuleId::preDistortion, 0});
        else if (tab == PluginHeader::Tab::post)
            scopeContext.setFocus(SlotId{ModuleId::postDistortion, 0});
        else if (tab == PluginHeader::Tab::main)
            scopeContext.setFocus(SlotId{ModuleId::main, activeSlot});

        const bool isMain = tab == PluginHeader::Tab::main;

        for (int i = 0; i < MainRouting::maxSlots; ++i)
            mainModules[(size_t) i]->setVisible(isMain && i == activeSlot);

        startModule->setVisible(tab == PluginHeader::Tab::start);

        preModule->setVisible(tab == PluginHeader::Tab::pre);
        postModule->setVisible(tab == PluginHeader::Tab::post);
        modulationRouting.setVisible(tab == PluginHeader::Tab::mod);

        routingPanel.setVisible(isMain);

        if (isMain)
        {
            mainModules[(size_t) activeSlot]->setLowerContent(&routingPanel, routingPanel.isFlush());
            updateRoutingAccent();
        }

        resized();
    }

    void resized() override{
        // the modulation row and the boxes under it are the editor's, across the whole width
        const auto boxBounds = getLocalBounds();

        for (auto& module : mainModules)
            module->setBounds(boxBounds);

        startModule->setBounds(boxBounds);
        preModule->setBounds(boxBounds);
        postModule->setBounds(boxBounds);
        modulationRouting.setBounds(boxBounds);
    }

private:
    void updateRoutingAccent() {
        routingPanel.setAccentColour(mainModules[(size_t) activeSlot]->getAccent());
    }

    ScopeContext& scopeContext;

    RoutingPanel routingPanel;
    // the routing table is the MOD page. the sources are the editor's, in the row it opens and shuts
    ModulationPage modulationRouting;

    std::array<std::unique_ptr<Module>, MainRouting::maxSlots> mainModules;
    std::unique_ptr<Module> preModule = nullptr;
    std::unique_ptr<Module> postModule = nullptr;
    std::unique_ptr<Module> startModule = nullptr;

    int activeSlot = 0;
    PluginHeader::Tab view = PluginHeader::Tab::main;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SaturationColumn)
};