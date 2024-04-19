#include "PresetManager.h"

Preset::PresetManager::PresetManager(juce::AudioProcessorValueTreeState &apvts) : valueTreeState(apvts)
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

/**
 * @brief Save a preset to the default directory.
 * Returns true if the preset was saved successfully, false otherwise.
*/
bool Preset::PresetManager::savePreset(const juce::String &presetName, const juce::String &author, const juce::String &description)
{
	if (presetName.isEmpty())
		return false;

	currentPreset.setValue(presetName);
	currentAuthor.setValue(author);
	const auto xml = valueTreeState.copyState().createXml();
	const auto presetFile = juce::File(defaultDirectory.getFullPathName() + "/User/" + presetName + "." + extension);
	if (presetFile.existsAsFile())
	{
		DBG("Preset file " + presetFile.getFullPathName() + " already exists");

		return false;
	}
	else
	{
		presetFile.create();
	}

	if (!xml->writeTo(presetFile))
	{
		DBG("Could not create preset file: " + presetFile.getFullPathName());
		jassertfalse;
		return false;
	}
	
	currentPreset.setValue(presetFile.getRelativePathFrom(defaultDirectory));

	return true;
}

void Preset::PresetManager::deletePreset(const juce::File &presetFile)
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
	currentAuthor.setValue("");
}

void Preset::PresetManager::loadPreset(const juce::File &presetFile)
{
	if (!presetFile.existsAsFile())
	{
		DBG("Preset file " + presetFile.getFullPathName() + " does not exist");
		jassertfalse;
		return;
	}

	juce::XmlDocument xmlDocument{presetFile};
	const auto valueTreeToLoad = juce::ValueTree::fromXml(*xmlDocument.getDocumentElement());

	valueTreeState.replaceState(valueTreeToLoad);
	currentPreset.setValue(presetFile.getRelativePathFrom(defaultDirectory));
	// currentAuthor.setValue(valueTreeToLoad.getChildWithName("author").getProperty("author", ""));
}

juce::File Preset::PresetManager::loadNextPreset()
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

juce::File Preset::PresetManager::loadPreviousPreset()
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

juce::Array<juce::File> Preset::PresetManager::getAllPresets() const
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
void Preset::PresetManager::recursiveSortedTraverse(const juce::File &directory, std::shared_ptr<juce::OwnedArray<Preset::PresetFile>> files)
{
	
	auto wildcard = juce::WildcardFileFilter("*." + extension, "*", "*");
	auto dirsFiles = directory.findChildFiles(juce::File::TypesOfFileToFind::findFilesAndDirectories, false);

	std::sort(dirsFiles.begin(), dirsFiles.end(), [](const juce::File &a, const juce::File &b)
			  { return a.isDirectory() && !b.isDirectory(); });

	for (int i = 0; i < dirsFiles.size(); i++)
	{
		if (dirsFiles[i].isDirectory())
		{
			auto preset = std::make_unique<PresetFile>(dirsFiles[i]);

			files->add(std::move(preset)); // add the directory as well as the children to the 2d array

			recursiveSortedTraverse(dirsFiles[i], files);
		}
		else if (wildcard.isFileSuitable(dirsFiles[i]))
		{
			auto preset = std::make_unique<PresetFile>(dirsFiles[i]);
			files->add(std::move(preset));
		}
	}

	return;
}

std::shared_ptr<juce::OwnedArray<Preset::PresetFile>> Preset::PresetManager::getPresetFileHierarchy()
{
	auto files = std::make_shared<juce::OwnedArray<Preset::PresetFile>>();

	recursiveSortedTraverse(defaultDirectory, files);

	this->presetsCache = files;
	
	return files;
}

juce::File Preset::PresetManager::getCurrentPreset() const
{
	return defaultDirectory.getChildFile(currentPreset.toString());
}

void Preset::PresetManager::valueTreeRedirected(juce::ValueTree &treeWhichHasBeenChanged)
{
	currentPreset.referTo(treeWhichHasBeenChanged.getPropertyAsValue(presetPathProperty, nullptr));
	currentAuthor.referTo(treeWhichHasBeenChanged.getPropertyAsValue("author", nullptr));
}