#pragma once

 
#include "Modules/Module.h"
#include "Modules/Panels/MBCompPanel.h"
#include "Modules/Panels/MSCompPanel.h"
#include "Modules/Panels/StereoCompPanel.h"
#include "Modules/Panels/ErosionPanel.h"
#include "Modules/Panels/SizzlePanel.h"
#include "Modules/Panels/ReductionPanel.h"
#include "Modules/Panels/JeffPanel.h"
#include "Modules/Panels/EmptyPanel.h"
#include "Modules/Panels/AllPassPanel.h"

#include "LookAndFeel/Palette.h"

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
        compander = std::make_unique<Module>(p, "COMP", "compressionOn", "compressionType", std::move(companderPanels));
        compander->setLookAndFeel(&knobLAF3);
        addAndMakeVisible(compander.get());

        std::vector<std::unique_ptr<Panel>> noisePanels;
        // ORDERING IS VERY IMPORTANT
        noisePanels.push_back(std::make_unique<SizzlePanel>(p));
        noisePanels.push_back(std::make_unique<ErosionPanel>(p));
        noisePanels.push_back(std::make_unique<ReductionPanel>(p, "BIT"));
        noisePanels.push_back(std::make_unique<ReductionPanel>(p, "SOFT BIT"));
        noisePanels.push_back(std::make_unique<JeffPanel>(p));

        noise = std::make_unique<Module>(p, "NOISE", "noiseDistortionEnabled", "noiseDistortionType", std::move(noisePanels));
        noise->setLookAndFeel(&knobLAF1);
        addAndMakeVisible(noise.get());

        std::vector<std::unique_ptr<Panel>> allpassPanels;
        // ORDERING IS VERY IMPORTANT
        allpassPanels.push_back(std::make_unique<AllPassPanel>(p));
        allpass = std::make_unique<Module>(p, "ALLPASS", "preDistortionEnabled", "", std::move(allpassPanels));
        allpass->setLookAndFeel(&knobLAF2);
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

        compander->setBounds(bounds.removeFromTop(height / 2));
        noise->setBounds(bounds.removeFromBottom(height / 4));
        allpass->setBounds(bounds);
    }

private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableButtonAttachment = nullptr;

    std::unique_ptr<Module> compander = nullptr;
    std::unique_ptr<Module> noise = nullptr;
    std::unique_ptr<Module> allpass = nullptr;

    KnobLAF knobLAF1 = KnobLAF(Palette::colours[1]);
    KnobLAF knobLAF2 = KnobLAF(Palette::colours[2]);
    KnobLAF knobLAF3 = KnobLAF(Palette::colours[3]);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LeftColumn)
};