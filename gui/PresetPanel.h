#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

#include "LookAndFeel/ComboBoxLookAndFeel.h"

class PresetPanel : public Component, Button::Listener, ComboBox::Listener
{
public:
	PresetPanel(PresetManager& pm) : presetManager(pm),
		saveButton("Save", DrawableButton::ImageOnButtonBackground),
		deleteButton("Delete", DrawableButton::ImageOnButtonBackground),
		previousPresetButton("Previous", DrawableButton::ImageOnButtonBackground),
		nextPresetButton("Next", DrawableButton::ImageOnButtonBackground)
	{
		

		auto saveIcon = makeIcon(R"svgDELIM(
		<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" fill="none" stroke="white" stroke-linecap="round" stroke-linejoin="round" stroke-width="2" class="lucide lucide-save"><path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z"/><path d="M17 21v-8H7v8M7 3v5h8"/></svg>
		)svgDELIM");
		saveButton.setImages(saveIcon.get());

		auto deleteIcon = makeIcon(R"svgDELIM(
		<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" fill="none" stroke="white" stroke-linecap="round" stroke-linejoin="round" stroke-width="2" class="lucide lucide-trash-2"><path d="M3 6h18M19 6v14c0 1-1 2-2 2H7c-1 0-2-1-2-2V6M8 6V4c0-1 1-2 2-2h4c1 0 2 1 2 2v2M10 11v6M14 11v6"/></svg>
		)svgDELIM");
		deleteButton.setImages(deleteIcon.get());

		auto leftChevronIcon = makeIcon(R"svgDELIM(
			<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="lucide lucide-chevron-left"><path d="m15 18-6-6 6-6"/></svg>
		)svgDELIM");
		previousPresetButton.setImages(leftChevronIcon.get());

		auto rightChevronIcon = makeIcon(R"svgDELIM(
			<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="lucide lucide-chevron-right"><path d="m9 18 6-6-6-6"/></svg>
		)svgDELIM");
		nextPresetButton.setImages(rightChevronIcon.get());

		setupButton(saveButton, "Save");
		setupButton(deleteButton, "Delete");
		setupButton(previousPresetButton, "<");
		setupButton(nextPresetButton, ">");


		presetList.setLookAndFeel(&comboBoxLAF);
		presetList.setJustificationType(Justification::centred);
		presetList.setColour(ComboBox::backgroundColourId, Colours::black);

		presetList.setTextWhenNothingSelected("--- Presets ---");
		presetList.setMouseCursor(MouseCursor::PointingHandCursor);
		addAndMakeVisible(presetList);
		presetList.addListener(this);

		loadPresetList();
	}

	~PresetPanel()
	{
		saveButton.removeListener(this);
		deleteButton.removeListener(this);
		previousPresetButton.removeListener(this);
		nextPresetButton.removeListener(this);
		presetList.removeListener(this);
	}

	void resized() override
	{
		const auto container = getLocalBounds().reduced(4).withTrimmedLeft(4).withTrimmedRight(4);
		auto bounds = container;

		saveButton.setBounds(bounds.removeFromLeft(getHeight()).reduced(4) );
		deleteButton.setBounds(bounds.removeFromLeft(getHeight()).reduced(4) );
		nextPresetButton.setBounds(bounds.removeFromRight(getHeight()).reduced(4) );
		previousPresetButton.setBounds(bounds.removeFromRight(getHeight()).reduced(4) );
		presetList.setBounds(bounds);
	}
private:
	void paint(juce::Graphics &g) override
    {
        Path p;
        p.addRoundedRectangle(getLocalBounds().reduced(4).toFloat(), 15.0f);
        g.setColour(juce::Colour::fromRGB(0, 0, 0));
        g.fillPath(p);
    }

	std::unique_ptr<juce::Drawable> makeIcon(const char *iconString) {
		auto parsedIconString {XmlDocument::parse (String (iconString))};
        jassert(parsedIconString != nullptr);
        auto drawableLogoString = Drawable::createFromSVG (*parsedIconString);
        jassert(drawableLogoString != nullptr);

		return drawableLogoString;
	}

	void buttonClicked(Button* button) override
	{
		if (button == &saveButton)
		{
			fileChooser = std::make_unique<FileChooser>(
				"Save Preset",
				PresetManager::defaultDirectory,
				"*." + PresetManager::extension
			);
			fileChooser->launchAsync(FileBrowserComponent::saveMode, [&](const FileChooser& chooser)
				{
					const auto resultFile = chooser.getResult();
					presetManager.savePreset(resultFile.getFileNameWithoutExtension());
					loadPresetList();
				});
		}
		if (button == &previousPresetButton)
		{
			const auto index = presetManager.loadPreviousPreset();
			presetList.setSelectedItemIndex(index, dontSendNotification);
		}
		if (button == &nextPresetButton)
		{
			const auto index = presetManager.loadNextPreset();
			presetList.setSelectedItemIndex(index, dontSendNotification);
		}
		if (button == &deleteButton)
		{
			presetManager.deletePreset(presetManager.getCurrentPreset());
			loadPresetList();
		}
	}
	void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override
	{
		if (comboBoxThatHasChanged == &presetList)
		{
			presetManager.loadPreset(presetList.getItemText(presetList.getSelectedItemIndex()));
		}
	}

	void setupButton(Button& button, const String& buttonText) 
	{
		juce::ignoreUnused(buttonText);
		// button.setButtonText(buttonText);
		button.setMouseCursor(MouseCursor::PointingHandCursor);

		button.setColour(TextButton::ColourIds::buttonOnColourId, Colour::fromRGB(0, 0, 0));
		button.setColour(TextButton::ColourIds::buttonColourId, Colour::fromRGB(0, 0, 0));
		button.setColour(TextButton::ColourIds::textColourOffId, Colour::fromRGB(255, 255, 255));
		button.setColour(ComboBox::outlineColourId, Colour::fromRGB(0, 0, 0));


		addAndMakeVisible(button);
		button.addListener(this);
	}

	void loadPresetList()
	{
		presetList.clear(dontSendNotification);
		const auto allPresets = presetManager.getAllPresets();
		const auto currentPreset = presetManager.getCurrentPreset();
		presetList.addItemList(allPresets, 1);
		presetList.setSelectedItemIndex(allPresets.indexOf(currentPreset), dontSendNotification);
	}

	PresetManager& presetManager;
	DrawableButton saveButton, deleteButton, previousPresetButton, nextPresetButton;
	ComboBox presetList;
	std::unique_ptr<FileChooser> fileChooser;

	ComboBoxLookAndFeel comboBoxLAF;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetPanel)
};