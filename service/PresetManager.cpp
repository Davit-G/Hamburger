#include "PresetManager.h"

const juce::File PresetManager::defaultDirectory{juce::File::getSpecialLocation(
													 juce::File::SpecialLocationType::commonDocumentsDirectory)
													 .getChildFile(JucePlugin_Manufacturer)
													 .getChildFile(JucePlugin_Name)};
const juce::String PresetManager::extension{"borgir"};
const juce::String PresetManager::presetPathProperty{"presetPath"};

PresetManager::PresetManager(juce::AudioProcessorValueTreeState &apvts) : valueTreeState(apvts)
{
	// Create a default Preset Directory, if it doesn't exist
	if (!defaultDirectory.exists())
	{
		// todo: load defaults here?

		const auto result = defaultDirectory.createDirectory();
		if (result.failed())
		{
			DBG("Could not create preset directory: " + result.getErrorMessage());
			jassertfalse;
		}
	}

	// does a child path called /user exist?
	auto child = juce::File(defaultDirectory.getFullPathName() + "/User");

	if (!child.exists())
	{
		const auto result = child.createDirectory();
		if (result.failed())
		{
			DBG("Could not create preset directory: " + result.getErrorMessage());
			jassertfalse;
		}
	}

	valueTreeState.state.addListener(this);
	currentPreset.referTo(valueTreeState.state.getPropertyAsValue(presetPathProperty, nullptr));
}

void PresetManager::savePreset(const juce::String &presetName)
{
	if (presetName.isEmpty())
		return;

	currentPreset.setValue(presetName);
	const auto xml = valueTreeState.copyState().createXml();
	const auto presetFile = juce::File(defaultDirectory.getFullPathName() + "/User/" + presetName + "." + extension);
	if (presetFile.existsAsFile())
	{
		DBG("Preset file " + presetFile.getFullPathName() + " already exists");
		return;
	} else {
		presetFile.create();
	}

	if (!xml->writeTo(presetFile))
	{
		DBG("Could not create preset file: " + presetFile.getFullPathName());
		jassertfalse;
	}
}

void PresetManager::deletePreset(const juce::File &presetFile)
{
	if (!presetFile.existsAsFile())
	{
		DBG("Preset file " + presetFile.getFullPathName() + " does not exist");
		jassertfalse;
		return;
	}
	if (!presetFile.deleteFile())
	{
		DBG("Preset file " + presetFile.getFullPathName() + " could not be deleted");
		jassertfalse;
		return;
	}
	currentPreset.setValue("");
}

void PresetManager::loadPreset(const juce::File &presetFile)
{
	if (!presetFile.existsAsFile())
	{
		DBG("Preset file " + presetFile.getFullPathName() + " does not exist");
		jassertfalse;
		return;
	}
	// presetFile (XML) -> (ValueTree)
	juce::XmlDocument xmlDocument{presetFile};
	const auto valueTreeToLoad = juce::ValueTree::fromXml(*xmlDocument.getDocumentElement());

	

	valueTreeState.replaceState(valueTreeToLoad);
	currentPreset.setValue(presetFile.getRelativePathFrom(defaultDirectory));
}

juce::File PresetManager::loadNextPreset()
{
	const auto allPresets = getAllPresets();
	if (allPresets.isEmpty())
		return juce::File();

	auto currentIndex = 0;

	for (int i = 0; i < allPresets.size(); i++)
	{
		if (allPresets[i].getRelativePathFrom(defaultDirectory) == currentPreset.toString())
		{
			DBG("Current preset: " + currentPreset.toString());
			DBG("Index: " + juce::String(i));
			currentIndex = i;
			break;
		}
	}

	const auto nextIndex = currentIndex + 1 > (allPresets.size() - 1) ? 0 : currentIndex + 1;
	const auto preset = allPresets.getReference(nextIndex);
	loadPreset(preset);
	return preset;
}

juce::File PresetManager::loadPreviousPreset()
{
	const auto allPresets = getAllPresets();
	if (allPresets.isEmpty())
		return juce::File();

	auto currentIndex = 0;

	for (int i = 0; i < allPresets.size(); i++)
	{
		if (allPresets[i].getRelativePathFrom(defaultDirectory) == currentPreset.toString())
		{
			DBG("Current preset: " + currentPreset.toString());
			DBG("Index: " + juce::String(i));
			currentIndex = i;
			break;
		}
	}

	const auto previousIndex = currentIndex - 1 < 0 ? allPresets.size() - 1 : currentIndex - 1;
	const auto preset = allPresets.getReference(previousIndex);
	loadPreset(preset);
	return preset;
}

juce::Array<juce::File> PresetManager::getAllPresets() const
{
	juce::StringArray presets;
	const auto fileArray = defaultDirectory.findChildFiles(
		juce::File::TypesOfFileToFind::findFiles, true, "*." + extension);

	return fileArray;
}

/**
 * @brief Recursively traverse a directory and return a sorted list of files.
 * Makes sure that folders are traversed before files.
 * Also filters by extension.
 */
juce::Array<juce::File> PresetManager::recursiveSortedTraverse(const juce::File &directory) const
{
	juce::Array<juce::File> files;

	auto wildcard = juce::WildcardFileFilter("*." + extension, "*", "*");
	auto dirsFiles = directory.findChildFiles(juce::File::TypesOfFileToFind::findFilesAndDirectories, false);

	// sort folders before directories
	std::sort(dirsFiles.begin(), dirsFiles.end(), [](const juce::File &a, const juce::File &b)
			  { return a.isDirectory() && !b.isDirectory(); });

	// for every folder, recursively traverse and insert
	for (int i = 0; i < dirsFiles.size(); i++)
	{
		if (dirsFiles[i].isDirectory())
		{
			auto subFiles = recursiveSortedTraverse(dirsFiles[i]);
			files.add(dirsFiles[i]); // add the directory as well as the children to the 2d arr
			files.addArray(subFiles);
		}
		else
		{
			if (wildcard.isFileSuitable(dirsFiles[i]))
			{
				files.add(dirsFiles[i]);
			}
		}
	}

	return files;
}

juce::Array<juce::File> PresetManager::getPresetFileHierarchy() const
{
	// get files and folders
	auto dirsFiles = recursiveSortedTraverse(defaultDirectory);

	return dirsFiles;
}

juce::File PresetManager::getCurrentPreset() const
{
	return defaultDirectory.getChildFile(currentPreset.toString());
}

void PresetManager::valueTreeRedirected(juce::ValueTree &treeWhichHasBeenChanged)
{
	currentPreset.referTo(treeWhichHasBeenChanged.getPropertyAsValue(presetPathProperty, nullptr));
}