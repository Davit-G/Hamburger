#pragma once

#include <JuceHeader.h>
#include "Modules/Module.h"
#include "Modules/Panels/CompanderPanel.h"
#include "Modules/Panels/ErosionPanel.h"
#include "Modules/Panels/SizzlePanel.h"
#include "Modules/Panels/ReductionPanel.h"
#include "Modules/Panels/JeffPanel.h"
#include "Modules/Panels/EmptyPanel.h"
#include "Modules/Panels/AllPassPanel.h"


class LeftColumn : public juce::Component
{
public:
    LeftColumn(AudioPluginAudioProcessor &p) : processorRef(p)
    {
        // panel with elements already inside
        std::vector<std::unique_ptr<Panel>> companderPanels;
        // ORDERING IS VERY IMPORTANT
        companderPanels.push_back(std::make_unique<CompanderPanel>(p));
        compander = std::make_unique<Module>(p, "COMPANDER", "compressionOn", "", std::move(companderPanels));
        addAndMakeVisible(compander.get());

        std::vector<std::unique_ptr<Panel>> noisePanels;
        // ORDERING IS VERY IMPORTANT
        noisePanels.push_back(std::make_unique<SizzlePanel>(p));
        noisePanels.push_back(std::make_unique<ErosionPanel>(p));
        noisePanels.push_back(std::make_unique<ReductionPanel>(p, "BIT"));
        noisePanels.push_back(std::make_unique<ReductionPanel>(p, "SOFT BIT"));
        noisePanels.push_back(std::make_unique<JeffPanel>(p));
        noise = std::make_unique<Module>(p, "NOISE", "noiseDistortionEnabled", "noiseDistortionType", std::move(noisePanels));
        addAndMakeVisible(noise.get());

        std::vector<std::unique_ptr<Panel>> allpassPanels;
        // ORDERING IS VERY IMPORTANT
        allpassPanels.push_back(std::make_unique<AllPassPanel>(p));
        allpass = std::make_unique<Module>(p, "ALLPASS", "preDistortionEnabled", "", std::move(allpassPanels));
        addAndMakeVisible(allpass.get());

    }

    ~LeftColumn() override{}

    void paint(Graphics &g) override
    {
        juce::ignoreUnused(g);
    }

    void resized() override{
        auto bounds = getLocalBounds();
        auto height = bounds.getHeight();

        compander->setBounds(bounds.removeFromTop(height / 2.f));
        noise->setBounds(bounds.removeFromBottom(height / 4.f));
        allpass->setBounds(bounds);
    }

private:
    AudioPluginAudioProcessor &processorRef;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableButtonAttachment = nullptr;

    std::unique_ptr<Module> compander;
    std::unique_ptr<Module> noise;
    std::unique_ptr<Module> allpass;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LeftColumn)
};