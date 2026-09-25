#pragma once

#include "Module.h"

#include "Panels/ClassicSatPanel.h"
#include "Panels/TubeSatPanel.h"
#include "Panels/PhaseDistPanel.h"
#include "Panels/RubidiumSatPanel.h"
#include "Panels/TapeSatPanel.h"
#include "Panels/SlewSaturator.h"
#include "Panels/WaveshapePanel.h"
#include "SlotLevels.h"

inline std::vector<std::unique_ptr<Panel>> makeDistortionPanels (AudioPluginAudioProcessor& p, SlotId slot)
{
    std::vector<std::unique_ptr<Panel>> panels;

    panels.push_back (std::make_unique<ClassicSatPanel> (p, slot));
    panels.push_back (std::make_unique<TubeSatPanel> (p, slot));
    panels.push_back (std::make_unique<PhaseDistPanel> (p, slot));
    panels.push_back (std::make_unique<RubidiumSatPanel> (p, slot));
    panels.push_back (std::make_unique<TapeSatPanel> (p, slot));
    panels.push_back (std::make_unique<SlewRatePanel> (p, slot));
    panels.push_back (std::make_unique<WaveshapePanel> (p, slot));

    return panels;
}

inline std::unique_ptr<Module> makeDistortionModule (AudioPluginAudioProcessor& p, SlotId slot,
                                                     const juce::String& title)
{
    auto module = std::make_unique<Module> (p, title.toStdString(),
                                            slot.enabled().getParamID().toStdString(),
                                            slot.type().getParamID().toStdString(),
                                            makeDistortionPanels (p, slot));

    module->setFooter (std::make_unique<SlotLevels> (p, slot), SlotLevels::height);

    return module;
}
