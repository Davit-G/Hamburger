#pragma once

#include "juce_gui_basics/juce_gui_basics.h"
#include "juce_core/juce_core.h"
#include "juce_audio_processors/juce_audio_processors.h"

class PresetManager : juce::ValueTree::Listener
{
public:
	static const juce::File defaultDirectory;
	static const juce::String extension;
	static const juce::String presetNameProperty;

	PresetManager(juce::AudioProcessorValueTreeState&);

	void savePreset(const juce::String& presetName);
	void deletePreset(const juce::String& presetName);
	void loadPreset(const juce::String& presetName);
	int loadNextPreset();
	int loadPreviousPreset();
	juce::StringArray getAllPresets() const;
	juce::String getCurrentPreset() const;
private:
	void valueTreeRedirected(juce::ValueTree& treeWhichHasBeenChanged) override;

	juce::AudioProcessorValueTreeState& valueTreeState;
	juce::Value currentPreset;
};