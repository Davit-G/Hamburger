#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <JuceHeader.h>

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor &p) : AudioProcessorEditor(&p),
                                                                                                 processorRef(p)
{
    
    juce::Image powerOffImage = juce::ImageCache::getFromMemory(BinaryData::poweroff_png, BinaryData::poweroff_pngSize);
    juce::Image powerOnImage = juce::ImageCache::getFromMemory(BinaryData::poweron_png, BinaryData::poweron_pngSize);


    companderButton = std::make_unique<LightButton>(p, "compandingOn", powerOffImage, powerOnImage);
    addAndMakeVisible(companderButton.get());

    emphasisButton = std::make_unique<LightButton>(p, "emphasisOn", powerOffImage, powerOnImage);
    addAndMakeVisible(emphasisButton.get());

    compressorButton = std::make_unique<LightButton>(p, "compressionOn", powerOffImage, powerOnImage);
    addAndMakeVisible(compressorButton.get());

    expanderButton = std::make_unique<LightButton>(p, "expansionOn", powerOffImage, powerOnImage);
    addAndMakeVisible(expanderButton.get());

    bypassButton = std::make_unique<LightButton>(p, "hamburgerEnabled", powerOffImage, powerOnImage);
    addAndMakeVisible(bypassButton.get());

    autoGainButton = std::make_unique<LightButton>(p, "autoGain", powerOffImage, powerOnImage);
    addAndMakeVisible(autoGainButton.get());

    juce::Image smallKnobImage = juce::ImageCache::getFromMemory(BinaryData::ingain2_min_png, BinaryData::ingain2_min_pngSize);
    juce::Image knobImage = juce::ImageCache::getFromMemory(BinaryData::knob_min_png, BinaryData::knob_min_pngSize);
    juce::Image sizzleImage = juce::ImageCache::getFromMemory(BinaryData::sizzleknob_min_png, BinaryData::sizzleknob_min_pngSize);

    inputGainKnob = std::make_unique<FilmStripKnob>(p, "inputGain", "Input Gain", -24.0f, 24.0f, smallKnobImage, 38);
    addAndMakeVisible(inputGainKnob.get());
    outputGainKnob = std::make_unique<FilmStripKnob>(p, "outputGain", "Output Gain", -24.0f, 24.0f, smallKnobImage, 38);
    addAndMakeVisible(outputGainKnob.get());
    mixKnob = std::make_unique<FilmStripKnob>(p, "mix", "Mix", 0.0f, 100.0f, smallKnobImage, 38);
    addAndMakeVisible(mixKnob.get());

    emphasisLowKnob = std::make_unique<FilmStripKnob>(p, "emphasisLowGain", "Emphasis Low Gain", -18.0f, 18.0f, smallKnobImage, 38);
    addAndMakeVisible(emphasisLowKnob.get());
    emphasisMidKnob = std::make_unique<FilmStripKnob>(p, "emphasisMidGain", "Emphasis Mid Gain", -18.0f, 18.0f, smallKnobImage, 38);
    addAndMakeVisible(emphasisMidKnob.get());
    emphasisHighKnob = std::make_unique<FilmStripKnob>(p, "emphasisHighGain", "Emphasis Hi Gain", -18.0f, 18.0f, smallKnobImage, 38);
    addAndMakeVisible(emphasisHighKnob.get());
    emphasisLowFreqKnob = std::make_unique<FilmStripKnob>(p, "emphasisLowFreq", "Emphasis Low Frequency", 30.0f, 200.0f, smallKnobImage, 38);
    addAndMakeVisible(emphasisLowFreqKnob.get());
    emphasisMidFreqKnob = std::make_unique<FilmStripKnob>(p, "emphasisMidFreq", "Emphasis Mid Frequency", 500.0f, 3000.0f, smallKnobImage, 38);
    addAndMakeVisible(emphasisMidFreqKnob.get());
    emphasisHighFreqKnob = std::make_unique<FilmStripKnob>(p, "emphasisHighFreq", "Emphasis Hi Frequency", 6000.0f, 18000.0f, smallKnobImage, 38);
    addAndMakeVisible(emphasisHighFreqKnob.get());

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

    background = juce::ImageCache::getFromMemory(BinaryData::hamburgernew8_png, BinaryData::hamburgernew8_pngSize);


    oversamplingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.treeState, "oversampling", oversamplingComboBox);
    oversamplingComboBox.addItemList({"Off", "2x", "4x", "8x", "16x"}, 1);
    oversamplingComboBox.setSelectedId(1);

    addAndMakeVisible(oversamplingComboBox);

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
    companderButton->setBounds(40, 95, 18, 18);
    emphasisButton->setBounds(627, 96, 18, 18);
    compressorButton->setBounds(53, 383, 18, 18);
    expanderButton->setBounds(53, 404, 18, 18); 

    bypassButton->setBounds(641, 389, 18, 18);
    autoGainButton->setBounds(641, 410, 18, 18);

    inputGainKnob->setBounds(604, 320, inputGainKnob->getKnobSize(), inputGainKnob->getKnobSize());
    outputGainKnob->setBounds(718, 320, outputGainKnob->getKnobSize(), outputGainKnob->getKnobSize());
    mixKnob->setBounds(661, 320, mixKnob->getKnobSize(), mixKnob->getKnobSize());

    emphasisLowKnob->setBounds(604, 202, emphasisLowKnob->getKnobSize(), emphasisLowKnob->getKnobSize());
    emphasisMidKnob->setBounds(660, 202, emphasisMidKnob->getKnobSize(), emphasisMidKnob->getKnobSize());
    emphasisHighKnob->setBounds(717, 202, emphasisHighKnob->getKnobSize(), emphasisHighKnob->getKnobSize());
    emphasisLowFreqKnob->setBounds(603, 138, emphasisLowFreqKnob->getKnobSize(), emphasisLowFreqKnob->getKnobSize());
    emphasisMidFreqKnob->setBounds(659, 138, emphasisMidFreqKnob->getKnobSize(), emphasisMidFreqKnob->getKnobSize());
    emphasisHighFreqKnob->setBounds(716, 138, emphasisHighFreqKnob->getKnobSize(), emphasisHighFreqKnob->getKnobSize());

    saturationKnob->setBounds(328, 113, saturationKnob->getKnobSize(), saturationKnob->getKnobSize());
    sizzleKnob->setBounds(353, 301, sizzleKnob->getKnobSize(), sizzleKnob->getKnobSize());
    fuzzKnob->setBounds(470, 329, fuzzKnob->getKnobSize(), fuzzKnob->getKnobSize());
    foldKnob->setBounds(255, 329, foldKnob->getKnobSize(), foldKnob->getKnobSize());

    compAttackKnob->setBounds(32, 221, compAttackKnob->getKnobSize(), compAttackKnob->getKnobSize());
    compReleaseKnob->setBounds(109, 221, compReleaseKnob->getKnobSize(), compReleaseKnob->getKnobSize());
    compThresholdKnob->setBounds(34, 142, compThresholdKnob->getKnobSize(), compThresholdKnob->getKnobSize());
    compRatioKnob->setBounds(107, 142, compRatioKnob->getKnobSize(), compRatioKnob->getKnobSize());
    compOutKnob->setBounds(69, 292, compOutKnob->getKnobSize(), compOutKnob->getKnobSize());

    oversamplingComboBox.setBounds(64, 473, 64, 38);
}
