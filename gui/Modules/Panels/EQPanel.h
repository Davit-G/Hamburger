#pragma once

#include "../Panel.h"

class EQPanel : public Panel
{
public:
    EQPanel(AudioPluginAudioProcessor &p) : Panel(p, "EMPHASIS"),
                                            lowFreq(p, "FREQ", ParamIDs::emphasisLowFreq, ParamUnits::hz),
                                            highFreq(p, "FREQ", ParamIDs::emphasisHighFreq, ParamUnits::hz),
                                            lowGain(p, "GAIN", ParamIDs::emphasisLowGain, ParamUnits::db),
                                            highGain(p, "GAIN", ParamIDs::emphasisHighGain, ParamUnits::db)
    {

        addAndMakeVisible(lowFreq);
        addAndMakeVisible(highFreq);
        addAndMakeVisible(lowGain);
        addAndMakeVisible(highGain);

        band1.setText("Low", juce::NotificationType::dontSendNotification);
        band2.setText("High", juce::NotificationType::dontSendNotification);

        band1.setJustificationType(juce::Justification::centred);
        band2.setJustificationType(juce::Justification::centred);

        addAndMakeVisible(band1);
        addAndMakeVisible(band2);

        Palette::setKnobColoursOfComponent(&lowFreq, Palette::colours[2]);
        Palette::setKnobColoursOfComponent(&highFreq, Palette::colours[2]);
        Palette::setKnobColoursOfComponent(&lowGain, Palette::colours[2]);
        Palette::setKnobColoursOfComponent(&highGain, Palette::colours[2]);
    }

    void resized()
    {
        auto bounds = getLocalBounds();

        auto headerTitles = bounds.removeFromTop(20);

        band1.setBounds(headerTitles.removeFromLeft(headerTitles.getWidth() / 2));
        band2.setBounds(headerTitles);

        using fr = juce::Grid::Fr;
        using Track = juce::Grid::TrackInfo;

        grid.templateRows = {Track(fr(1)), Track(fr(1))};
        grid.templateColumns = {Track(fr(1)), Track(fr(1))};

        grid.items = {
            juce::GridItem(lowFreq).withArea(1, 1),
            juce::GridItem(highFreq).withArea(1, 2),
            juce::GridItem(lowGain).withArea(2, 1),
            juce::GridItem(highGain).withArea(2, 2)};

        grid.performLayout(bounds);
    }

    void paint(juce::Graphics &g) override
    {
        // draw line down the middle

        g.setColour(juce::Colour::fromRGB(44, 44, 44));

        int height = 100;

        g.drawLine(
            getWidth() / 2 - 1, getHeight() / 2 - height / 2,
            getWidth() / 2 - 1, getHeight() / 2 + height / 2,
            2);
    }

private:
    juce::Grid grid;

    juce::Label band1;
    juce::Label band2;

    ParamKnob lowFreq;
    ParamKnob highFreq;
    ParamKnob lowGain;
    ParamKnob highGain;
};