#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <JuceHeader.h>

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor &p) : AudioProcessorEditor(&p),
                                                                                                 processorRef(p)
{
    juce::ignoreUnused(processorRef);
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    

    

    juce::Image smallKnobImage = juce::ImageCache::getFromMemory(BinaryData::ingain_min_png, BinaryData::ingain_min_pngSize);
    juce::Image knobImage = juce::ImageCache::getFromMemory(BinaryData::knob_min_png, BinaryData::knob_min_pngSize);
    juce::Image sizzleImage = juce::ImageCache::getFromMemory(BinaryData::sizzleknob_min_png, BinaryData::sizzleknob_min_pngSize);
    // juce::Image foldFuzzImage = juce::ImageCache::getFromMemory(BinaryData::knob_min_png, BinaryData::knob);

    inputGainKnob = std::make_unique<FilmStripKnob>(p, "inputGain", "Input Gain", -24.0f, 24.0f, smallKnobImage, 35);
    addAndMakeVisible(inputGainKnob.get());
    outputGainKnob = std::make_unique<FilmStripKnob>(p, "outputGain", "Output Gain", -24.0f, 24.0f, smallKnobImage, 35);
    addAndMakeVisible(outputGainKnob.get());
    mixKnob = std::make_unique<FilmStripKnob>(p, "mix", "Mix", 0.0f, 100.0f, smallKnobImage, 35);
    addAndMakeVisible(mixKnob.get());

    emphasisLowKnob = std::make_unique<FilmStripKnob>(p, "emphasisLowGain", "Emphasis Low Gain", -18.0f, 18.0f, smallKnobImage, 35);
    addAndMakeVisible(emphasisLowKnob.get());
    emphasisMidKnob = std::make_unique<FilmStripKnob>(p, "emphasisMidGain", "Emphasis Mid Gain", -18.0f, 18.0f, smallKnobImage, 35);
    addAndMakeVisible(emphasisMidKnob.get());
    emphasisHighKnob = std::make_unique<FilmStripKnob>(p, "emphasisHighGain", "Emphasis Hi Gain", -18.0f, 18.0f, smallKnobImage, 35);
    addAndMakeVisible(emphasisHighKnob.get());

    fuzzKnob = std::make_unique<FilmStripKnob>(p, "fuzz", "Fuzz", 0.0f, 100.f, knobImage, 54);
    addAndMakeVisible(fuzzKnob.get());
    sizzleKnob = std::make_unique<FilmStripKnob>(p, "sizzle", "Sizzle", 0.0f, 100.f, sizzleImage, 72);
    addAndMakeVisible(sizzleKnob.get());
    foldKnob = std::make_unique<FilmStripKnob>(p, "fold", "Fold", 0.0f, 100.f, knobImage, 54);
    addAndMakeVisible(foldKnob.get());
    saturationKnob = std::make_unique<FilmStripKnob>(p, "saturation", "Saturation", 0.0f, 100.f, juce::ImageCache::getFromMemory(BinaryData::saturation_knob_min_png, BinaryData::saturation_knob_min_pngSize), 128);
    addAndMakeVisible(saturationKnob.get());

    compAttackKnob = std::make_unique<FilmStripKnob>(p, "compAttack", "Compander Attack", 3.0f, 200.f, knobImage, 54);
    addAndMakeVisible(compAttackKnob.get());
    compReleaseKnob = std::make_unique<FilmStripKnob>(p, "compRelease", "Compander Release", 10.0f, 500.f, knobImage, 54);
    addAndMakeVisible(compReleaseKnob.get());
    compThresholdKnob = std::make_unique<FilmStripKnob>(p, "compThreshold", "Compander Threshold", -48.0f, 0.f, knobImage, 54);
    addAndMakeVisible(compThresholdKnob.get());
    compRatioKnob = std::make_unique<FilmStripKnob>(p, "compRatio", "Compander Ratio", 1.0f, 10.f, knobImage, 54);
    addAndMakeVisible(compRatioKnob.get());
    compOutKnob = std::make_unique<FilmStripKnob>(p, "compOut", "Compander Compressor Out", 0.0f, 24.0f, knobImage, 54);
    addAndMakeVisible(compOutKnob.get());

    // attach button to parameter
    buttonValue = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processorRef.treeState, "compOn", companderButton);
    // button.setButtonText("Enable Compander");
    addAndMakeVisible(companderButton);

    // attach button to parameter
    emphasisValue = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processorRef.treeState, "emphasisOn", emphasisButton);
    // button.setButtonText("Enable Compander");
    addAndMakeVisible(emphasisButton);

    background = juce::ImageCache::getFromMemory(BinaryData::hamburgernew6_png, BinaryData::hamburgernew6_pngSize);
    setSize(778, 600);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{

}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint(juce::Graphics &g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    // g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // (Our component is opaque, so we must completely fill the background with a solid colour)
    
    g.drawImageAt(background, 0, 0);
}

void AudioPluginAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto localBounds = getLocalBounds();
    companderButton.setBounds(46, 91, 50, 50);
    emphasisButton.setBounds(106, 91, 50, 50);

    inputGainKnob->setBounds(606, 363, inputGainKnob->getKnobSize(), inputGainKnob->getKnobSize());
    outputGainKnob->setBounds(719, 363, outputGainKnob->getKnobSize(), outputGainKnob->getKnobSize());
    mixKnob->setBounds(662, 363, mixKnob->getKnobSize(), mixKnob->getKnobSize());

    emphasisLowKnob->setBounds(606, 156, emphasisLowKnob->getKnobSize(), emphasisLowKnob->getKnobSize());
    emphasisMidKnob->setBounds(662, 156, emphasisMidKnob->getKnobSize(), emphasisMidKnob->getKnobSize());
    emphasisHighKnob->setBounds(720, 156, emphasisHighKnob->getKnobSize(), emphasisHighKnob->getKnobSize());

    saturationKnob->setBounds(328, 113, saturationKnob->getKnobSize(), saturationKnob->getKnobSize());
    sizzleKnob->setBounds(353, 301, sizzleKnob->getKnobSize(), sizzleKnob->getKnobSize());
    fuzzKnob->setBounds(470, 329, fuzzKnob->getKnobSize(), fuzzKnob->getKnobSize());
    foldKnob->setBounds(255, 329, foldKnob->getKnobSize(), foldKnob->getKnobSize());

    compAttackKnob->setBounds(36, 276, compAttackKnob->getKnobSize(), compAttackKnob->getKnobSize());
    compReleaseKnob->setBounds(112, 276, compReleaseKnob->getKnobSize(), compReleaseKnob->getKnobSize());
    compThresholdKnob->setBounds(74, 119, compThresholdKnob->getKnobSize(), compThresholdKnob->getKnobSize());
    compRatioKnob->setBounds(74, 197, compRatioKnob->getKnobSize(), compRatioKnob->getKnobSize());
    compOutKnob->setBounds(74, 356, compOutKnob->getKnobSize(), compOutKnob->getKnobSize());
}
