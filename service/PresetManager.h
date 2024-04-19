#pragma once

#include "juce_gui_basics/juce_gui_basics.h"
#include "juce_core/juce_core.h"
#include "juce_audio_processors/juce_audio_processors.h"

namespace Preset
{

	static const juce::File defaultDirectory{juce::File::getSpecialLocation(
												 juce::File::SpecialLocationType::commonDocumentsDirectory)
												 .getChildFile(JucePlugin_Manufacturer)
												 .getChildFile(JucePlugin_Name)};
	static const juce::String extension{"borgir"};
	static const juce::String presetPathProperty{"presetPath"};

	class PresetFile
	{
	public:
		PresetFile(juce::File &file) : file(file)
		{
			id = file.getRelativePathFrom(defaultDirectory);

			loadMetadata();
		}

		void loadMetadata()
		{
			juce::XmlDocument xmlDocument{file};
			const auto valueTreeToLoad = juce::ValueTree::fromXml(*xmlDocument.getDocumentElement());
			author = valueTreeToLoad.getProperty("author");
		}

		juce::String getAuthor() const
		{
			return author;
		}

		juce::String getId() const
		{
			return id;
		}

		juce::File getFile() const
		{
			return file;
		}

	private:
		juce::File &file;

		juce::String author;
		juce::String id;
	};

	class PresetManager : juce::ValueTree::Listener
	{
	public:
		PresetManager(juce::AudioProcessorValueTreeState &);

		void savePreset(const juce::String &preset, const juce::String &author);
		void deletePreset(const juce::File &preset);
		void loadPreset(const juce::File &preset);
		juce::File loadNextPreset();
		juce::File loadPreviousPreset();
		juce::Array<juce::File> getAllPresets() const;
		juce::File getCurrentPreset() const;

		juce::Array<juce::File> recursiveSortedTraverse(const juce::File &directory) const;

		juce::Array<juce::File> getPresetFileHierarchy() const;

	private:
		void valueTreeRedirected(juce::ValueTree &treeWhichHasBeenChanged) override;

		juce::AudioProcessorValueTreeState &valueTreeState;
		juce::Value currentPreset;
		juce::Value currentAuthor;
	};

}