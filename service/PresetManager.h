#pragma once

#include "juce_gui_basics/juce_gui_basics.h"
#include "juce_core/juce_core.h"
#include "juce_audio_processors/juce_audio_processors.h"

namespace Preset
{

	static const juce::File defaultDirectory{juce::File::getSpecialLocation(
												 juce::File::SpecialLocationType::userDocumentsDirectory)
												 .getChildFile(JucePlugin_Manufacturer)
												 .getChildFile(JucePlugin_Name)
												 .getChildFile("./presets/")};
	static const juce::String extension{"borgir"};
	static const juce::String presetPathProperty{"presetPath"};

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
			// description = valueTreeToLoad.getProperty("description");
		}

		juce::String getDescription() const
		{
			return description;
		}

		juce::String getAuthor() const
		{
			return author;
		}

		juce::String getId() const
		{
			return id;
		}

		juce::File getFile()
		{
			return file;
		}

	private:
		juce::File file;

		juce::String author;
		juce::String description;
		juce::String id;
	};

	class PresetManager : juce::ValueTree::Listener
	{
	public:
		PresetManager(juce::AudioProcessorValueTreeState &);

		bool savePreset(const juce::String &preset, const juce::String &author, const juce::String &description, std::function<void(std::string)> cb);
		void deletePreset(const juce::File &preset, std::function<void(std::string)> cb);
		void loadPreset(const juce::File &preset, std::function<void(std::string)> cb);

		bool saveFile(const juce::File &presetFile, std::function<void(std::string)> cb);

		juce::String getCurrentPresetName() const;
		juce::String getCurrentAuthor() const;

		void setPresetDirectory(const juce::File &directory);
		juce::File getPresetDirectory() const;
		
		juce::File loadNextPreset(std::function<void(std::string)> cb);
		juce::File loadPreviousPreset(std::function<void(std::string)> cb);
		juce::Array<juce::File> getAllPresets() const;
		juce::File getCurrentPreset() const;

		void recursiveSortedTraverse(const juce::File &directory, std::shared_ptr<juce::OwnedArray<Preset::PresetFile>> presets);

		std::shared_ptr<juce::OwnedArray<Preset::PresetFile>> getPresetFileHierarchy();

		juce::ApplicationProperties appProperties;
	private:
		void valueTreeRedirected(juce::ValueTree &treeWhichHasBeenChanged) override;

		std::shared_ptr<juce::OwnedArray<Preset::PresetFile>> presetsCache = nullptr;

		juce::AudioProcessorValueTreeState &valueTreeState;
		juce::Value currentPreset;
		juce::Value currentAuthor;

		juce::PropertiesFile::Options options;

		// this is different to the default one, which is selected when we dont have a custom preset folder
		std::unique_ptr<juce::File> customPresetDirectory = nullptr;
	};

}