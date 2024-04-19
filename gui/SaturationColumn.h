#pragma once 

#include "Modules/Module.h"

#include "Modules/Panels/ClassicSatPanel.h"
#include "Modules/Panels/MatrixSatPanel.h"
#include "Modules/Panels/TubeSatPanel.h"
#include "Modules/Panels/RubidiumSatPanel.h" 
#include "Modules/Panels/PhaseDistPanel.h"
#include "Modules/Panels/PostClipPanel.h"
#include "Modules/Panels/ErosionPanel.h"
#include "Modules/Panels/SizzlePanel.h"
#include "Modules/Panels/ReductionPanel.h"
#include "Modules/Panels/GatePanel.h"

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

        classic = std::make_unique<ClassicSatPanel>(p);
        classic->setLookAndFeel(&saturationLAF);
        panels.push_back(std::move(classic));

        tube = std::make_unique<TubeSatPanel>(p);
        tube->setLookAndFeel(&tubeSatLAF);
        panels.push_back(std::move(tube));

        // auto waveshape = std::make_unique<WaveshaperPanel>(p);
        // waveshape->setLookAndFeel(&tubeSatLAF);
        // panels.push_back(std::move(waveshape));

        phase = std::make_unique<PhaseDistPanel>(p);
        phase->setLookAndFeel(&phaseLAF); // todo: customise
        panels.push_back(std::move(phase));

        rubidium = std::make_unique<RubidiumSatPanel>(p);
        rubidium->setLookAndFeel(&rubidiumLAF); // todo: customise
        panels.push_back(std::move(rubidium));

        matrixSat = std::make_unique<MatrixSatPanel>(p);
        matrixSat->setLookAndFeel(&matrixLAF);
        panels.push_back(std::move(matrixSat));

        saturation = std::make_unique<Module>(p, "DISTORTION", "primaryDistortionEnabled", "primaryDistortionType", std::move(panels));
        addAndMakeVisible(saturation.get());

        std::vector<std::unique_ptr<Panel>> clipPanel;

        postClipPanel = std::make_unique<PostClipPanel>(p);
        postClipPanel->setLookAndFeel(&tubeSatLAF);
        clipPanel.push_back(std::move(postClipPanel));

        postClip = std::make_unique<Module>(p, "CLIPPER", "postClipEnabled", "", std::move(clipPanel));
        addAndMakeVisible(postClip.get());

        std::vector<std::unique_ptr<Panel>> noisePanels;
        // ORDERING IS VERY IMPORTANT
        noisePanels.push_back(std::make_unique<SizzlePanel>(p));
        noisePanels.push_back(std::make_unique<ErosionPanel>(p));
        noisePanels.push_back(std::make_unique<ReductionPanel>(p));
        noisePanels.push_back(std::make_unique<GatePanel>(p));
        noisePanels.push_back(std::make_unique<SizzleOGPanel>(p));

        noise = std::make_unique<Module>(p, "NOISE", "noiseDistortionEnabled", "noiseDistortionType", std::move(noisePanels));
        noise->setLookAndFeel(&knobLAF1);
        addAndMakeVisible(noise.get());
    }

    void resized() override{
        auto bounds = getLocalBounds();
        auto height = bounds.getHeight();

        saturation->setBounds(bounds.removeFromTop(height * 3/4));
        postClip->setBounds(bounds.removeFromRight(bounds.getWidth() / 2));
        noise->setBounds(bounds);
    }

private:
    KnobLAF saturationLAF = KnobLAF(Palette::colours[0]);
    KnobLAF tubeSatLAF = KnobLAF(Palette::colours[4]);
    KnobLAF phaseLAF = KnobLAF(Palette::colours[3]);
    KnobLAF rubidiumLAF = KnobLAF(Palette::colours[2]);
    KnobLAF matrixLAF = KnobLAF(Palette::colours[6]);
    KnobLAF knobLAF1 = KnobLAF(Palette::colours[1]);

    std::unique_ptr<Panel> classic = nullptr;
    std::unique_ptr<Panel> tube = nullptr;
    std::unique_ptr<Panel> phase = nullptr;
    std::unique_ptr<Panel> rubidium = nullptr;
    std::unique_ptr<Panel> matrixSat = nullptr;
    std::unique_ptr<Panel> postClipPanel = nullptr;

    std::unique_ptr<Module> noise = nullptr;
    std::unique_ptr<Module> saturation;
    std::unique_ptr<Module> postClip;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SaturationColumn)
};