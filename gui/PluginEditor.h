#pragma once

#include "../PluginProcessor.h"
#include <JuceHeader.h>

#include "LeftColumn.h"
#include "SaturationColumn.h"
#include "UtilColumn.h"

using namespace juce;

class EditorV2 : public juce::AudioProcessorEditor
{
public:
    EditorV2(AudioPluginAudioProcessor &p) : AudioProcessorEditor(&p),
                                             processorRef(p),
                                             leftColumn(p),
                                             saturationColumn(p),
                                             utilColumn(p)
    {
        setSize(1000, 500);

        addAndMakeVisible(leftColumn);
        addAndMakeVisible(saturationColumn);
        addAndMakeVisible(utilColumn);
    }

    ~EditorV2() override{}

    void paint(juce::Graphics &g)
    {
        auto image = ImageCache::getFromMemory(BinaryData::bg3_jpg, BinaryData::bg3_jpgSize);
        g.drawImage(image, getLocalBounds().toFloat());
    }

    void resized()
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
    AudioPluginAudioProcessor &processorRef;

    LeftColumn leftColumn;
    SaturationColumn saturationColumn;
    UtilColumn utilColumn;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditorV2)
};