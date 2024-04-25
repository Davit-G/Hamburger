#pragma once

#include "../PluginProcessor.h"
#include "juce_gui_extra/juce_gui_extra.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "juce_core/juce_core.h"
#include "juce_audio_processors/juce_audio_processors.h"

#include "BinaryData.h"

#include "Info.h"
#include "LeftColumn.h"
#include "SaturationColumn.h"
#include "UtilColumn.h"

#include "PresetPanel.h"

#include "LookAndFeel/HamburgerLAF.h"

#define PRESETS_MANAGER_VISIBLE 1

class EditorV2 : public juce::AudioProcessorEditor
{
public:
    EditorV2(AudioPluginAudioProcessor &p) : AudioProcessorEditor(&p),
                                             leftColumn(p),
                                             saturationColumn(p),
                                             utilColumn(p),
                                             infoPanel(p)
                                             
#if PRESETS_MANAGER_VISIBLE 
                                             ,presetPanel(p.getPresetManager())
#endif
    {
        int additionalHeight = 0;

        setLookAndFeel(&hamburgerLAF);
        infoPanel.setLookAndFeel(&hamburgerLAF);
        leftColumn.setLookAndFeel(&hamburgerLAF);
        saturationColumn.setLookAndFeel(&hamburgerLAF);
        utilColumn.setLookAndFeel(&hamburgerLAF);

        addAndMakeVisible(leftColumn);
        addAndMakeVisible(saturationColumn);
        addAndMakeVisible(utilColumn);
        addAndMakeVisible(infoPanel);

#if PRESETS_MANAGER_VISIBLE
        addAndMakeVisible(presetPanel);
        additionalHeight += 45;
#endif

        setOpaque(true);

        infoPanel.setVisible(false);

        setPaintingIsUnclipped(true);
        

        setSize(800, 500 + additionalHeight);
    }

    ~EditorV2()
    {
        setLookAndFeel(nullptr);
        infoPanel.setLookAndFeel(nullptr);
        leftColumn.setLookAndFeel(nullptr);
        saturationColumn.setLookAndFeel(nullptr);
        utilColumn.setLookAndFeel(nullptr);
    }

    void paint(juce::Graphics &g) override
    {
        g.drawImage(image, getLocalBounds().toFloat(), juce::RectanglePlacement::fillDestination);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto totalWidth = bounds.getWidth() / 4;

        infoPanel.setBounds(bounds);

#if PRESETS_MANAGER_VISIBLE
        presetPanel.setBounds(bounds);
        bounds.removeFromTop(45);
#endif
        auto left = bounds.removeFromLeft(totalWidth);
        auto right = bounds.removeFromRight(totalWidth);

        leftColumn.setBounds(left);
        saturationColumn.setBounds(bounds);
        utilColumn.setBounds(right);
    }

    void handleCommandMessage(int command) override
    {
        if (command == 0)
        {
            infoPanel.setVisible(true);

            leftColumn.setVisible(false);
            saturationColumn.setVisible(false);
            utilColumn.setVisible(false);
        }
        else if (command == 1)
        {
            infoPanel.setVisible(false);

            leftColumn.setVisible(true);
            saturationColumn.setVisible(true);
            utilColumn.setVisible(true);
        }

#if PRESETS_MANAGER_VISIBLE
        if (command == 0)
        {
            presetPanel.setVisible(false);
        }
        else if (command == 1)
        {
            presetPanel.setVisible(true);
        }
#endif
    }

private:
    LeftColumn leftColumn;
    SaturationColumn saturationColumn;
    UtilColumn utilColumn;

    HamburgerLAF hamburgerLAF;

#if PRESETS_MANAGER_VISIBLE
    PresetPanel presetPanel;
#endif

    Info infoPanel;

    juce::Image image = juce::ImageCache::getFromMemory(BinaryData::bg4_jpg, BinaryData::bg4_jpgSize);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditorV2)
};