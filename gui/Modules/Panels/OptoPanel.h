#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"
#include "../../Knob.h"
#include "../../RectSlider.h"

#include "../../SaturationIcons/CentredSVGIcon.h"

// the opto distortion, where the LEDs clip the audio as well as light the cell
class OptoPanel : public Panel
{
public:
    OptoPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::main, 0}) : Panel(p, "OPTO", &Theme::opto, slot),
        drive(p, "DRIVE", slot, ParamIDs::optoDrive, ScopeContextType::IN_OUT),
        bias(p, "DC BIAS", slot, ParamIDs::optoBias, ScopeContextType::IN_OUT),
        dcSpeed(p, "DC SPEED", slot, ParamIDs::optoDcSpeed, ScopeContextType::IN_OUT),
        damping(p, "DAMPING", slot, ParamIDs::optoDamping, ScopeContextType::IN_OUT),
        icon(BinaryData::Opto_svg, BinaryData::Opto_svgSize, &Theme::opto)
    {
        for (auto* control : std::initializer_list<juce::Component*> { &drive, &bias, &dcSpeed, &damping, &icon })
            addAndMakeVisible(control);
    }

    void resized() override
    {
        if (usesCompactLayout())
            compactLayout(drive, icon, {&bias, &dcSpeed, &damping});
        else
            fourKnobLayout(drive, icon, bias, dcSpeed, damping);
    }

private:
    ParamKnob drive;
    RectSlider bias, dcSpeed, damping;
    CentredSVGIcon icon;
};

// the opto compressors, where the audio only lights the cells, in one band with a stereo link or three with a threshold tilt
class OptoCompPanel : public Panel
{
public:
    OptoCompPanel(AudioPluginAudioProcessor &p, bool multiband)
        : Panel(p, multiband ? "MB OPTO" : "OPTO", multiband ? &Theme::multibandOptoComp : &Theme::optoComp),
        threshold(p, "THRES", compSlot, ParamIDs::optoThreshold, ScopeContextType::COMPRESSION),
        ratio(p, "RATIO", compSlot, ParamIDs::optoRatio, ScopeContextType::COMPRESSION),
        speed(p, "SPEED", compSlot, ParamIDs::optoCompSpeed, ScopeContextType::COMPRESSION),
        linkOrTilt(p, multiband ? "TILT" : "LINK", compSlot, multiband ? ParamIDs::compBandTilt : ParamIDs::compStereoLink, ScopeContextType::COMPRESSION)
    {
        for (auto* knob : { &threshold, &ratio, &speed, &linkOrTilt })
            addAndMakeVisible(knob);
    }

    void resized() override
    {
        using fr = juce::Grid::Fr;
        using Track = juce::Grid::TrackInfo;

        grid.templateRows = {Track(fr(1)), Track(fr(1))};
        grid.templateColumns = {Track(fr(1)), Track(fr(1))};

        grid.items = {
            juce::GridItem(threshold).withArea(1, 1),
            juce::GridItem(ratio).withArea(1, 2),
            juce::GridItem(speed).withArea(2, 1),
            juce::GridItem(linkOrTilt).withArea(2, 2)};

        grid.performLayout(getLocalBounds());
    }

private:
    static constexpr SlotId compSlot { ModuleId::dynamics, 0 };

    juce::Grid grid;

    ParamKnob threshold, ratio, speed, linkOrTilt;
};
