#pragma once

#include "../PluginProcessor.h"
#include <JuceHeader.h>

#include "LeftColumn.h"
#include "SaturationColumn.h"
#include "UtilColumn.h"

#include "melatonin_inspector/melatonin_inspector.h"

using namespace juce;

class EditorV2 : public juce::AudioProcessorEditor
{
public:
    EditorV2(AudioPluginAudioProcessor &p) : AudioProcessorEditor(&p),
                                             leftColumn(p),
                                             saturationColumn(p),
                                             utilColumn(p)
    {
        setSize(800, 500);

        addAndMakeVisible(leftColumn);
        addAndMakeVisible(saturationColumn);
        addAndMakeVisible(utilColumn);

        setOpaque(true);
        // setResizable(true, true);

        setPaintingIsUnclipped(true);
    }

    ~EditorV2() override {}

    void paint(juce::Graphics &g) override
    {
        g.drawImage(image, getLocalBounds().toFloat(), RectanglePlacement::fillDestination);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto totalWidth = bounds.getWidth() / 4;

        auto left = bounds.removeFromLeft(totalWidth);
        auto right = bounds.removeFromRight(totalWidth);

        leftColumn.setBounds(left);
        saturationColumn.setBounds(bounds);
        utilColumn.setBounds(right);
    }

private:
    LeftColumn leftColumn;
    SaturationColumn saturationColumn;
    UtilColumn utilColumn;

    juce::Image image = ImageCache::getFromMemory(BinaryData::bg4_jpg, BinaryData::bg4_jpgSize);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditorV2)
};