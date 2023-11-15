#include <JuceHeader.h>

#include "Modules/Module.h"

#include "Modules/Panels/ClassicSatPanel.h"
#include "Modules/Panels/TubeSatPanel.h"

class SaturationColumn : public juce::Component
{
public:
    SaturationColumn(AudioPluginAudioProcessor &p) : processorRef(p) {
        std::vector<std::unique_ptr<Panel>> panels;
        // ORDERING IS VERY IMPORTANT
        panels.push_back(std::make_unique<ClassicSatPanel>(p));
        panels.push_back(std::make_unique<TubeSatPanel>(p));
        saturation = std::make_unique<Module>(p, "SATURATION", "primaryDistortionEnabled", "primaryDistortionType", std::move(panels));
        addAndMakeVisible(saturation.get());
    }

    ~SaturationColumn() override{}

    void resized() override{
        auto bounds = getLocalBounds();
        auto height = bounds.getHeight();

        saturation->setBounds(bounds);
    }

private:
    AudioPluginAudioProcessor &processorRef;

    std::unique_ptr<Module> saturation;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SaturationColumn)
};