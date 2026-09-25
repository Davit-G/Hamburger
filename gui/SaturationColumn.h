#pragma once 

#include "Modules/Module.h"

#include "Modules/DistortionModule.h"
#include "PluginHeader.h"
#include "Modules/Panels/PostClipPanel.h"
#include "Modules/Panels/ErosionPanel.h"
#include "Modules/Panels/SizzlePanel.h"
#include "Modules/Panels/ReductionPanel.h"
#include "Modules/Panels/GatePanel.h"
#include "Modules/Panels/SlewSaturator.h"

#include "LookAndFeel/Palette.h"

#include "Modules/ClipIndicator.h"
#include "Modules/Panels/RoutingPanel.h"
#include "SettingsPanel.h"

class SaturationColumn : public juce::Component
{
public:
    SaturationColumn(AudioPluginAudioProcessor &p) : scopeContext(p.getScopeContext()), clipDot(p.getScopeDataCollector(), p), routingPanel(p), settingsPanel(p) {
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
        addChildComponent(settingsPanel);

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

    ~SaturationColumn() {
        postClip->setLookAndFeel(nullptr);
        noise->setLookAndFeel(nullptr);
    }

    // the settings' fx order names each module in its box's colour, and some of those boxes live in other columns
    void setModuleColours(std::function<juce::Colour(ModuleId)> colourFor) {
        settingsPanel.getFxOrder().colourFor = std::move(colourFor);
    }

    // the box on screen for a module this column holds, nullptr for the rest
    Module* moduleFor(ModuleId id) {
        switch (id) {
            case ModuleId::main:           return mainModules[(size_t) activeSlot].get();
            case ModuleId::preDistortion:  return preModule.get();
            case ModuleId::postDistortion: return postModule.get();
            case ModuleId::module1:        return noise.get();
            case ModuleId::postClip:       return postClip.get();
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

        preModule->setVisible(tab == PluginHeader::Tab::pre);
        postModule->setVisible(tab == PluginHeader::Tab::post);
        settingsPanel.setVisible(tab == PluginHeader::Tab::settings);
        
        routingPanel.setVisible(isMain);

        if (isMain)
        {
            mainModules[(size_t) activeSlot]->setLowerContent(&routingPanel, routingPanel.isFlush());
            updateRoutingAccent();
        }

        resized();
    }

    void resized() override{
        auto bounds = getLocalBounds();
        auto height = bounds.getHeight();

        auto boxBounds = bounds.removeFromTop(height * 3/4);

        for (auto& module : mainModules)
            module->setBounds(boxBounds);

        preModule->setBounds(boxBounds);
        postModule->setBounds(boxBounds);
        settingsPanel.setBounds(boxBounds);

        
        auto postClipBounds = bounds.removeFromRight(bounds.getWidth() / 2);
        postClip->setBounds(postClipBounds);
        clipDot.setBounds(postClipBounds.removeFromTop(dotSize).removeFromRight(dotSize).reduced(4).translated(-19, 19));
        
        noise->setBounds(bounds);
    }

private:
    void updateRoutingAccent() {
        routingPanel.setAccentColour(mainModules[(size_t) activeSlot]->getAccentColour());
    }

    ScopeContext& scopeContext;

    RoutingPanel routingPanel;
    SettingsPanel settingsPanel;

    std::unique_ptr<Panel> postClipPanel = nullptr;

    std::array<std::unique_ptr<Module>, MainRouting::maxSlots> mainModules;
    std::unique_ptr<Module> preModule = nullptr;
    std::unique_ptr<Module> postModule = nullptr;

    std::unique_ptr<Module> noise = nullptr;
    std::unique_ptr<Module> postClip = nullptr;

    int activeSlot = 0;
    PluginHeader::Tab view = PluginHeader::Tab::main;

    static constexpr int dotSize = 16;
    ClipIndicator clipDot;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SaturationColumn)
};