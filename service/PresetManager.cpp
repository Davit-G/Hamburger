#include "PresetManager.h"

const juce::File PresetManager::defaultDirectory{juce::File::getSpecialLocation(
													 juce::File::SpecialLocationType::commonDocumentsDirectory)
													 .getChildFile(JucePlugin_Manufacturer)
													 .getChildFile(JucePlugin_Name)};
const juce::String PresetManager::extension{"borgir"};
const juce::String PresetManager::presetNameProperty{"presetName"};

PresetManager::PresetManager(juce::AudioProcessorValueTreeState &apvts) : valueTreeState(apvts)
{
	// Create a default Preset Directory, if it doesn't exist
	if (!defaultDirectory.exists())
	{
		const auto result = defaultDirectory.createDirectory();
		if (result.failed())
		{
			DBG("Could not create preset directory: " + result.getErrorMessage());
			jassertfalse;
		}
	}

	valueTreeState.state.addListener(this);
	currentPreset.referTo(valueTreeState.state.getPropertyAsValue(presetNameProperty, nullptr));
}

void PresetManager::savePreset(const juce::String &presetName)
{
	if (presetName.isEmpty())
		return;

	currentPreset.setValue(presetName);
	const auto xml = valueTreeState.copyState().createXml();
	const auto presetFile = defaultDirectory.getChildFile(presetName + "." + extension);
	if (!xml->writeTo(presetFile))
	{
		DBG("Could not create preset file: " + presetFile.getFullPathName());
		jassertfalse;
	}
}

void PresetManager::deletePreset(const juce::String &presetName)
{
	if (presetName.isEmpty())
		return;

	const auto presetFile = defaultDirectory.getChildFile(presetName + "." + extension);
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

void PresetManager::loadPreset(const juce::String &presetName)
{
	if (presetName.isEmpty())
		return;

	const auto presetFile = defaultDirectory.getChildFile(presetName + "." + extension);
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
	currentPreset.setValue(presetName);
}

int PresetManager::loadNextPreset()
{
	const auto allPresets = getAllPresets();
	if (allPresets.isEmpty())
		return -1;
	const auto currentIndex = allPresets.indexOf(currentPreset.toString());
	const auto nextIndex = currentIndex + 1 > (allPresets.size() - 1) ? 0 : currentIndex + 1;
	loadPreset(allPresets.getReference(nextIndex));
	return nextIndex;
}

int PresetManager::loadPreviousPreset()
{
	const auto allPresets = getAllPresets();
	if (allPresets.isEmpty())
		return -1;
	const auto currentIndex = allPresets.indexOf(currentPreset.toString());
	const auto previousIndex = currentIndex - 1 < 0 ? allPresets.size() - 1 : currentIndex - 1;
	loadPreset(allPresets.getReference(previousIndex));
	return previousIndex;
}

juce::StringArray PresetManager::getAllPresets() const
{
	juce::StringArray presets;
	const auto fileArray = defaultDirectory.findChildFiles(
		juce::File::TypesOfFileToFind::findFiles, false, "*." + extension);
	for (const auto &file : fileArray)
	{
		presets.add(file.getFileNameWithoutExtension());
	}
	return presets;
}

juce::String PresetManager::getCurrentPreset() const
{
	return currentPreset.toString();
}

void PresetManager::valueTreeRedirected(juce::ValueTree &treeWhichHasBeenChanged)
{
	currentPreset.referTo(treeWhichHasBeenChanged.getPropertyAsValue(presetNameProperty, nullptr));
}