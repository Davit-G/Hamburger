#pragma once

#include "juce_gui_basics/juce_gui_basics.h"
#include "juce_core/juce_core.h"
#include "juce_audio_processors/juce_audio_processors.h"

class PresetManager : juce::ValueTree::Listener
{
public:
	static const juce::File defaultDirectory;
	static const juce::String extension;
	static const juce::String presetPathProperty;

	PresetManager(juce::AudioProcessorValueTreeState&);

	void savePreset(const juce::String& preset, const juce::String &author);
	void deletePreset(const juce::File& preset);
	void loadPreset(const juce::File& preset);
	juce::File loadNextPreset();
	juce::File loadPreviousPreset();
	juce::Array<juce::File> getAllPresets() const;
	juce::File getCurrentPreset() const;

	juce::Array<juce::File> recursiveSortedTraverse(const juce::File& directory) const;

	juce::Array<juce::File> getPresetFileHierarchy() const;
private:
	void valueTreeRedirected(juce::ValueTree& treeWhichHasBeenChanged) override;

	juce::AudioProcessorValueTreeState& valueTreeState;
	juce::Value currentPreset;
	juce::Value currentAuthor;
};