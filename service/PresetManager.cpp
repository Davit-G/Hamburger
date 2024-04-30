#include "PresetManager.h"

/**
 * Compare version strings. Return 1 if the first string is greater than the second, -1 if the first string is less than the second, and 0 if they are equal.
 */
int compareVersionStrings(juce::String v1, juce::String v2)
{
	auto v1Parts = juce::StringArray::fromTokens(v1, ".", "");
	auto v2Parts = juce::StringArray::fromTokens(v2, ".", "");

	for (int i = 0; i < std::min(v1Parts.size(), v2Parts.size()); i++)
	{
		auto v1Part = v1Parts[i].getIntValue();
		auto v2Part = v2Parts[i].getIntValue();

		if (v1Part > v2Part)
		{
			return 1;
		}
		else if (v1Part < v2Part)
		{
			return -1;
		}
	}

	if (v1Parts.size() > v2Parts.size())
	{
		return 1;
	}
	else if (v1Parts.size() < v2Parts.size())
	{
		return -1;
	}

	return 0;
}

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
bool Preset::PresetManager::savePreset(const juce::String &presetName, const juce::String &author, const juce::String &description, std::function<void(std::string)> cb)
{
	if (presetName.isEmpty())
	{
		DBG("Preset name is empty");
		cb("Preset name is empty");
		return false;
	}

	currentPreset.setValue(presetName);
	currentAuthor.setValue(author);
	
	valueTreeState.state.setProperty("version", JucePlugin_VersionString, nullptr);

	const auto xml = valueTreeState.copyState().createXml();
	const auto presetFile = juce::File(defaultDirectory.getFullPathName() + "/User/" + presetName + "." + extension);


	if (presetFile.existsAsFile())
	{
		DBG("Preset file " + presetFile.getFullPathName() + " already exists");
		cb(std::string("Preset file ") + presetFile.getFullPathName().toStdString() + " already exists");

		return false;
	}

	auto error = presetFile.create();

	if (!xml->writeTo(presetFile))
	{
		DBG("Could not create preset file: " + presetFile.getFullPathName());
		cb(std::string("Could not create preset file (") + presetFile.getFullPathName().toStdString() + "): " + error.getErrorMessage().toStdString());


		jassertfalse;
		return false;
	}

	currentPreset.setValue(presetFile.getRelativePathFrom(defaultDirectory));

	return true;
}

void Preset::PresetManager::deletePreset(const juce::File &presetFile, std::function<void(std::string)> cb)
{
	if (!presetFile.existsAsFile())
	{
		DBG("Preset file " + presetFile.getFullPathName() + " does not exist");
		cb(std::string("Preset file ") + presetFile.getFullPathName().toStdString() + " does not exist");
		jassertfalse;
		return;
	}

	if (!presetFile.deleteFile())
	{
		DBG("Preset file " + presetFile.getFullPathName() + " could not be deleted");
		cb(std::string("Preset file ") + presetFile.getFullPathName().toStdString() + " could not be deleted");
		jassertfalse;
		return;
	}

	currentPreset.setValue("");
	currentAuthor.setValue("");
}

void Preset::PresetManager::loadPreset(const juce::File &presetFile, std::function<void(std::string)> cb)
{
	auto relativePath = presetFile.getRelativePathFrom(defaultDirectory);

	if (!presetFile.existsAsFile())
	{
		DBG("Preset file " + presetFile.getFullPathName() + " does not exist");
		cb(std::string("Preset file ") + presetFile.getFullPathName().toStdString() + " does not exist");
		jassertfalse;
		return;
	}

	juce::XmlDocument xmlDocument{presetFile};
	const auto valueTreeToLoad = juce::ValueTree::fromXml(*xmlDocument.getDocumentElement());

	auto version = valueTreeToLoad.getProperty("version", "").toString();
	auto versionStd = version.toStdString();

	if (version == "") {
		DBG("Preset file " + relativePath.toStdString() + " is invalid (No version string)");
		cb(std::string("Preset file ") + relativePath.toStdString() + " is invalid (No version string)");
		return;
	}

	int compareVersions = compareVersionStrings(version, JucePlugin_VersionString);

	if (compareVersions > 0)
	{
		DBG("Preset file " + relativePath.toStdString() + " was saved with a newer version of the plugin (v" + versionStd + "). Preset will not be loaded.");
		cb(std::string("Preset file ") + relativePath.toStdString() + " was saved with a newer version of the plugin (v" + versionStd + "). Preset will not be loaded.");
		return;
	}
	else if (compareVersions < 0)
	{
		DBG("Preset file " + relativePath.toStdString() + " was saved with an older version of the plugin (v" + versionStd + "). Preset will sound different, but will be loaded anyway.");
		cb(std::string("Preset file ") + relativePath.toStdString() + " was saved with an older version of the plugin (v" + versionStd + "). Preset will sound different, but will be loaded anyway.");
	}

	valueTreeState.replaceState(valueTreeToLoad);
	currentPreset.setValue(relativePath);
	// currentAuthor.setValue(valueTreeToLoad.getChildWithName("author").getProperty("author", ""));
}

juce::File Preset::PresetManager::loadNextPreset(std::function<void(std::string)> cb)
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
	loadPreset(preset, cb);
	return preset;
}

juce::File Preset::PresetManager::loadPreviousPreset(std::function<void(std::string)> cb)
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
	loadPreset(preset, cb);
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
	if (currentPreset.getValue() == juce::var())
	{
		return juce::File();
	}

	return defaultDirectory.getChildFile(currentPreset.toString());
}

void Preset::PresetManager::valueTreeRedirected(juce::ValueTree &treeWhichHasBeenChanged)
{
	currentPreset.referTo(treeWhichHasBeenChanged.getPropertyAsValue(presetPathProperty, nullptr));
	currentAuthor.referTo(treeWhichHasBeenChanged.getPropertyAsValue("author", nullptr));
}