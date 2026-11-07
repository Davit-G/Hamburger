#pragma once

#include "juce_gui_basics/juce_gui_basics.h"
#include "juce_core/juce_core.h"
#include "juce_audio_processors/juce_audio_processors.h"

#include "./AppProperties.h"

namespace Preset
{

	static const juce::File defaultDirectory{juce::File::getSpecialLocation(
												 juce::File::SpecialLocationType::userDocumentsDirectory)
												 .getChildFile(JucePlugin_Manufacturer)
												 .getChildFile(JucePlugin_Name)
												 .getChildFile("./presets/")};
	static const juce::String extension{"burger"};
	static const juce::String presetPathProperty{"presetPath"};

	// the plugin version that saved a state, on every saved state: presets and projects both
	static const juce::Identifier versionProperty{"version"};

	/*	Loads a saved state, a preset or a project. Any parameter the state doesn't mention - one added since it was saved -
		goes back to its default, where replaceState on its own would leave it as it was. Then it's stamped with this
		version, so whatever is saved next says what wrote it. */
	void loadState(juce::AudioProcessorValueTreeState &state, const juce::ValueTree &saved);

	class PresetFile
	{
	public:
		PresetFile(juce::File file = juce::File()) : file(file)
		{
			// id = file.getRelativePathFrom(defaultDirectory);

			if (!file.existsAsFile())
			{
				DBG("Preset file " + file.getFullPathName() + " does not exist (presetfile constructor)");
				return;
			}

			if (file.getFileExtension() != "." + extension)
			{
				DBG("Preset file " + file.getFullPathName() + " does not have the correct extension");
				return;
			}
			
			if (file.isDirectory())
			{
				DBG("Preset file " + file.getFullPathName() + " is a directory");
				return;
			}

			// check if file is empty
			if (file.getSize() == 0)
			{
				DBG("Preset file " + file.getFullPathName() + " is empty");
				return;
			}

			try {
				loadMetadata();
			} catch (const std::exception &e) {
				DBG("Error loading metadata for preset file " + file.getFullPathName() + ": " + e.what());
			}
		}

		void loadMetadata()
		{
			auto thing = this->file;

			juce::XmlDocument xmlDocument{thing};

			const auto valueTreeToLoad = juce::ValueTree::fromXml(*xmlDocument.getDocumentElement());
			author = valueTreeToLoad.getProperty("author");
			description = valueTreeToLoad.getProperty("description");
		}

		juce::String getDescription() const
		{
			return description;
		}

		juce::String getAuthor() const
		{
			return author;
		}

		juce::File getFile() const
		{
			return file;
		}

	private:
		juce::File file;

		juce::String author;
		juce::String description;
	};

	class PresetManager : juce::ValueTree::Listener
	{
	public:
		PresetManager(juce::AudioProcessorValueTreeState &, AppProperties &);

		bool savePreset(const juce::String &preset, const juce::String &author, const juce::String &description, std::function<void(std::string)> cb);
		bool renamePreset(const juce::File &preset, const juce::String &newName, std::function<void(std::string)> cb);
		void deletePreset(const juce::File &preset, std::function<void(std::string)> cb);
		void loadPreset(const juce::File &preset, std::function<void(std::string)> cb);

		bool saveFile(const juce::File &presetFile, std::function<void(std::string)> cb);

		juce::String getCurrentPresetName() const;
		juce::String getCurrentAuthor() const;
		juce::String getCurrentDescription() const;
		juce::String getLastAuthor();

		void setPresetDirectory(const juce::File &directory);
		juce::File getPresetDirectory() const;
		
		juce::File loadNextPreset(std::function<void(std::string)> cb);
		juce::File loadPreviousPreset(std::function<void(std::string)> cb);
		juce::Array<juce::File> getAllPresets() const;
		juce::File getCurrentPreset() const;

		// the preset folder's own folders, one level deep, with the preset folder itself first when it holds presets too
		juce::Array<juce::File> getFolders() const;
		// the presets straight inside a folder, by name
		juce::Array<juce::File> getPresetsIn(const juce::File &folder) const;
	private:
		void valueTreeRedirected(juce::ValueTree &treeWhichHasBeenChanged) override;

		AppProperties& appProperties;

		juce::AudioProcessorValueTreeState &valueTreeState;
		juce::Value currentPreset;

		juce::PropertiesFile::Options options;

		// this is different to the default one, which is selected when we dont have a custom preset folder
		std::unique_ptr<juce::File> customPresetDirectory = nullptr;
	};

}