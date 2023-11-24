#pragma once 

#include "Modules/Module.h"

#include "Modules/Panels/ClassicSatPanel.h"
#include "Modules/Panels/TubeSatPanel.h"

#include "LookAndFeel/Palette.h"

class SaturationColumn : public juce::Component
{
public:
    SaturationColumn(AudioPluginAudioProcessor &p) {
        std::vector<std::unique_ptr<Panel>> panels;
        // ORDERING IS VERY IMPORTANT
        // code is weird so order of panels compared to order of DSP processing matters
        // TODO: fix this later

        // todo: add waveshapey distortion effects for distortion type, rename that to classic
        // and add rectification to it?

        auto classic = std::make_unique<ClassicSatPanel>(p);
        classic->setLookAndFeel(&saturationLAF);
        panels.push_back(std::move(classic));

        auto tube = std::make_unique<TubeSatPanel>(p);
        tube->setLookAndFeel(&tubeSatLAF);
        panels.push_back(std::move(tube));

        saturation = std::make_unique<Module>(p, "SATURATION", "primaryDistortionEnabled", "primaryDistortionType", std::move(panels));
        addAndMakeVisible(saturation.get());
    }

    ~SaturationColumn() override {}

    void resized() override{
        auto bounds = getLocalBounds();
        // auto height = bounds.getHeight();

        saturation->setBounds(bounds);
    }

private:
    KnobLAF saturationLAF = KnobLAF(Palette::colours[0]);
    KnobLAF tubeSatLAF = KnobLAF(Palette::colours[4]);

    std::unique_ptr<Module> saturation;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SaturationColumn)
};