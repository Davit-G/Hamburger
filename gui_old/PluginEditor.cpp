#include "../PluginProcessor.h"
#include "PluginEditor.h"

#include <JuceHeader.h>
#include "PreDistortionPanels/PreDistortions.h"

//==============================================================================
EditorV1::EditorV1(AudioPluginAudioProcessor &p) : AudioProcessorEditor(&p),
                                                                                                 processorRef(p),
                                                                                                 emphasisPanel(p),
                                                                                                 preDistortionsPanel(p),
                                                                                                 distortionsPanel(p),
                                                                                                 noiseTypesPanel(p)
{

    juce::Image powerOffImage = juce::ImageCache::getFromMemory(BinaryData::poweroff_png, BinaryData::poweroff_pngSize);
    juce::Image powerOnImage = juce::ImageCache::getFromMemory(BinaryData::poweron_png, BinaryData::poweron_pngSize);

    addAndMakeVisible(emphasisPanel);
    addAndMakeVisible(preDistortionsPanel);
    addAndMakeVisible(distortionsPanel);
    addAndMakeVisible(noiseTypesPanel);

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

    inputGainKnob = std::make_unique<FilmStripKnob>(p, "inputGain", "In", -24.0f, 24.0f, smallKnobImage, 38);
    addAndMakeVisible(inputGainKnob.get());
    outputGainKnob = std::make_unique<FilmStripKnob>(p, "outputGain", "Out", -24.0f, 24.0f, smallKnobImage, 38);
    addAndMakeVisible(outputGainKnob.get());
    mixKnob = std::make_unique<FilmStripKnob>(p, "mix", "Mix", 0.0f, 100.0f, smallKnobImage, 38);
    addAndMakeVisible(mixKnob.get());

    

    fuzzKnob = std::make_unique<FilmStripKnob>(p, "fuzz", "Fuzz", 0.0f, 100.f, knobImage, 54);
    // addAndMakeVisible(fuzzKnob.get());
    sizzleKnob = std::make_unique<FilmStripKnob>(p, "sizzle", "Sizzle", 0.0f, 100.f, sizzleImage, 72);
    // addAndMakeVisible(sizzleKnob.get());
    foldKnob = std::make_unique<FilmStripKnob>(p, "fold", "Fold", 0.0f, 100.f, knobImage, 54);
    // addAndMakeVisible(foldKnob.get());


    saturationKnob = std::make_unique<FilmStripKnob>(p, "saturationAmount", "Saturation", 0.0f, 100.f, sizzleImage, 72); //juce::ImageCache::getFromMemory(BinaryData::saturation_knob_min_png, BinaryData::saturation_knob_min_pngSize)
    // addAndMakeVisible(saturationKnob.get());

    compAttackKnob = std::make_unique<FilmStripKnob>(p, "compAttack", "Attack", 3.0f, 200.f, knobImage, 54);
    addAndMakeVisible(compAttackKnob.get());
    compReleaseKnob = std::make_unique<FilmStripKnob>(p, "compRelease", "Release", 10.0f, 500.f, knobImage, 54);
    addAndMakeVisible(compReleaseKnob.get());
    compThresholdKnob = std::make_unique<FilmStripKnob>(p, "compThreshold", "Threshold", -48.0f, 0.f, knobImage, 54);
    addAndMakeVisible(compThresholdKnob.get());
    compRatioKnob = std::make_unique<FilmStripKnob>(p, "compRatio", "Ratio", 1.0f, 10.f, knobImage, 54);
    addAndMakeVisible(compRatioKnob.get());
    compOutKnob = std::make_unique<FilmStripKnob>(p, "compOut", "Makeup", 0.0f, 24.0f, knobImage, 54);
    addAndMakeVisible(compOutKnob.get());


    oversamplingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.treeState, "oversamplingFactor", oversamplingComboBox);
    oversamplingComboBox.addItemList({"Off", "2x", "4x", "8x", "16x"}, 1);
    oversamplingComboBox.setSelectedItemIndex(0);
    oversamplingComboBox.setLookAndFeel(&comboBoxLook);
    addAndMakeVisible(oversamplingComboBox);

    // distortionTypesAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.treeState, "primaryDistortionType", distortionTypeComboBox);
    // distortionTypeComboBox.addItemList({ "Soft Clip", "Hard Clip", "Fold", "Fuzz", "Tube" }, 1);
    // distortionTypeComboBox.setSelectedItemIndex(0);
    // distortionTypeComboBox.setLookAndFeel(&comboBoxLook);
    // addAndMakeVisible(distortionTypeComboBox);

    background = juce::ImageCache::getFromMemory(BinaryData::hamburgernew13_png, BinaryData::hamburgernew13_pngSize);
    
    setSize(778, 600);
}

EditorV1::~EditorV1()
{
}

//==============================================================================
void EditorV1::paint(juce::Graphics &g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    // g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // (Our component is opaque, so we must completely fill the background with a solid colour)

    g.drawImageAt(background, 0, 0);
}

void EditorV1::resized()
{
    // In the future, a better ui would involve non-preprogrammed locations for knobs and stuff
    auto localBounds = getLocalBounds();
    companderButton->setBounds(40, 95, 18, 18);
    emphasisButton->setBounds(627, 96, 18, 18);
    compressorButton->setBounds(53, 383, 18, 18);
    expanderButton->setBounds(53, 404, 18, 18);



    emphasisPanel.setBounds(605, 116, 167, 164);
    preDistortionsPanel.setBounds(193, 83, 395, 126);
    distortionsPanel.setBounds(193, 209, 385, 126);
    noiseTypesPanel.setBounds(193, 332, 395, 113);

    bypassButton->setBounds(641, 389, 18, 18);
    autoGainButton->setBounds(641, 410, 18, 18);

    inputGainKnob->setBounds(604, 320, inputGainKnob->getKnobSize(), inputGainKnob->getKnobHeight());
    outputGainKnob->setBounds(718, 320, outputGainKnob->getKnobSize(), outputGainKnob->getKnobHeight());
    mixKnob->setBounds(661, 320, mixKnob->getKnobSize(), mixKnob->getKnobHeight());

    saturationKnob->setBounds(328, 213, saturationKnob->getKnobSize(), saturationKnob->getKnobHeight());
    sizzleKnob->setBounds(353, 301, sizzleKnob->getKnobSize(), sizzleKnob->getKnobHeight());
    fuzzKnob->setBounds(470, 329, fuzzKnob->getKnobSize(), fuzzKnob->getKnobHeight());
    foldKnob->setBounds(255, 329, foldKnob->getKnobSize(), foldKnob->getKnobHeight());

    compAttackKnob->setBounds(32, 221, compAttackKnob->getKnobSize(), compAttackKnob->getKnobHeight());
    compReleaseKnob->setBounds(109, 221, compReleaseKnob->getKnobSize(), compReleaseKnob->getKnobHeight());
    compThresholdKnob->setBounds(34, 142, compThresholdKnob->getKnobSize(), compThresholdKnob->getKnobHeight());
    compRatioKnob->setBounds(107, 142, compRatioKnob->getKnobSize(), compRatioKnob->getKnobHeight());
    compOutKnob->setBounds(69, 292, compOutKnob->getKnobSize(), compOutKnob->getKnobHeight());

    oversamplingComboBox.setBounds(60, 473, 80, 30);
    // distortionTypeComboBox.setBounds(630, 473, 80, 30);
}
