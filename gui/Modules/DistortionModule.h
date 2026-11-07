#pragma once

#include "Module.h"

#include "Panels/ClassicSatPanel.h"
#include "Panels/TubeSatPanel.h"
#include "Panels/PhaseDistPanel.h"
#include "Panels/RubidiumSatPanel.h"
#include "Panels/TapeSatPanel.h"
#include "Panels/SlewSaturator.h"
#include "Panels/WaveshapePanel.h"
#include "Panels/DllPanel.h"
#include "SlotLevels.h"

inline std::vector<std::unique_ptr<Panel>> makeDistortionPanels(AudioPluginAudioProcessor& p, SlotId slot)
{
    std::vector<std::unique_ptr<Panel>> panels;

    panels.push_back(std::make_unique<ClassicSatPanel>(p, slot));
    panels.push_back(std::make_unique<TubeSatPanel>(p, slot));
    panels.push_back(std::make_unique<PhaseDistPanel>(p, slot));
    panels.push_back(std::make_unique<RubidiumSatPanel>(p, slot));
    panels.push_back(std::make_unique<TapeSatPanel>(p, slot));
    panels.push_back(std::make_unique<SlewRatePanel>(p, slot));
    panels.push_back(std::make_unique<WaveshapePanel>(p, slot));
   #if JUCE_DEBUG
    panels.push_back(std::make_unique<DllPanel>(p, slot));
   #endif

    return panels;
}

inline std::unique_ptr<Module> makeDistortionModule(AudioPluginAudioProcessor& p, SlotId slot,
                                                     const juce::String& title)
{
    // the main boxes' power is the whole main section's, the band view has one for each band
    const auto power = slot.module == ModuleId::main ? ParamIDs::mainEnabled.getParamID() : slot.enabled().getParamID();
    auto module = std::make_unique<Module>(p, title.toStdString(),
                                            power.toStdString(),
                                            slot.type().getParamID().toStdString(),
                                            makeDistortionPanels(p, slot));

    module->setFooter(std::make_unique<SlotLevels>(p, slot), SlotLevels::height);

    // in the order of the type menu
    const auto& names = ParamIDs::distortionTypes.categories;
    module->categorySelector.tiles = {
        { names[0], BinaryData::Grill_svg, BinaryData::Grill_svgSize, &Theme::grill },
        { names[1], BinaryData::Tube_svg, BinaryData::Tube_svgSize, &Theme::tube },
        { names[2], BinaryData::Waves_svg, BinaryData::Waves_svgSize, &Theme::phase },
        { names[3], BinaryData::Flask_svg, BinaryData::Flask_svgSize, &Theme::rubidium },
        { names[4], BinaryData::FilmReel_svg, BinaryData::FilmReel_svgSize, &Theme::tape },
        { names[5], BinaryData::Slew_svg, BinaryData::Slew_svgSize, &Theme::slew },
        { names[6], nullptr, 0, &Theme::waveshape },
       #if JUCE_DEBUG
        { names[7], nullptr, 0, &Theme::stack },
       #endif
    };

    return module;
}
