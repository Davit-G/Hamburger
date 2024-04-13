#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

#include "LookAndFeel/ComboBoxLookAndFeel.h"
#include "BurgerAlertWindow.h"

struct FileComparator
{
	FileComparator(PresetManager &pm) : presetManager(pm) {}
	
	int compareElements (const File& a, const File& b) const
    {
        if (a.isDirectory() && !b.isDirectory()) return -1;
        if (!a.isDirectory() && b.isDirectory()) return 1;
        return 0;

		// we need directories on the same level to be sorted alphabetically while keeping directories at the top.
		// auto aDepth = a.getRelativePathFrom(presetManager.defaultDirectory).retainCharacters("/\\").length();
		// auto bDepth = b.getRelativePathFrom(presetManager.defaultDirectory).retainCharacters("/\\").length();

		

		
    }

	PresetManager &presetManager;
};

class CustomListBoxModel : public ListBoxModel
{
public:
	CustomListBoxModel(PresetManager &pm, ListBox &lb) : presetManager(pm), listBox(lb), comparator(pm)
	{
		filesFolders = presetManager.getPresetFileHierarchy();

		for (auto &file : filesFolders)
		{
			isCollapsed[file.getRelativePathFrom(pm.defaultDirectory)] = true;
		}

		refreshFilesToRender();
	}

	void refreshFilesToRender()
	{
		filesToRender.clear();
		for (auto &file : filesFolders)
		{
			auto relativePath = file.getRelativePathFrom(presetManager.defaultDirectory);
			auto depth = relativePath.retainCharacters("/\\").length();
			if (depth == 0)
			{
				filesToRender.add(file);
			}
			else
			{
				auto parent = file.getParentDirectory();
				File current = parent;

				int tries = 0;

				while (current != presetManager.defaultDirectory)
				{
					auto currentFile = current.getRelativePathFrom(presetManager.defaultDirectory);
					if (isCollapsed[currentFile] || tries++ > 40) break;

					current = current.getParentDirectory();
				}

				if (!isCollapsed[current.getRelativePathFrom(presetManager.defaultDirectory)])
				{
					filesToRender.add(file);
				}
			}
		}

		// push all directories to the top
		// filesToRender.sort(comparator, false);
	}

	int getNumRows() override
	{
		return filesToRender.size();
	}

	void paintListBoxItem(int rowNumber, Graphics &g,
						  int width, int height, bool rowIsSelected) override
	{

		auto row = filesToRender[rowNumber];

		auto isDir = row.isDirectory();

		if (rowIsSelected)
			g.fillAll(Colours::lightblue);

		
		auto depth = row.getRelativePathFrom(presetManager.defaultDirectory).retainCharacters("/\\").length();

		if (isDir)
		{
			g.fillAll(Colours::grey);
		}

		g.setColour(LookAndFeel::getDefaultLookAndFeel().findColour(Label::textColourId));
		g.setFont((float)height * 0.7f);

		g.drawText(row.getFileNameWithoutExtension(),
				   5 + depth * 30, 0, width, height,
				   Justification::centredLeft, true);
	}

	void listBoxItemClicked(int row, const MouseEvent &mouseEvent) override {
		auto item = filesToRender[row];

		auto isDir = item.isDirectory();

		if (isDir)
		{
			
			DBG("clicked on dir", item.getFullPathName());
			auto relativePath = item.getRelativePathFrom(presetManager.defaultDirectory);
			isCollapsed[relativePath] = !isCollapsed[relativePath];
			refreshFilesToRender();
			listBox.updateContent();
		}
		else
		{
			// presetManager.loadPreset(fileFolder.getFileNameWithoutExtension());
		}
	}

	void updateContent()
	{
		filesFolders = presetManager.getPresetFileHierarchy();
	}

private:
	PresetManager &presetManager;
	ListBox &listBox;

	FileComparator comparator;

	juce::Array<File> filesFolders;
	juce::Array<File> filesToRender;

	std::map<String, bool> isCollapsed;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomListBoxModel)
};

class PresetPanel : public Component, Button::Listener, ComboBox::Listener
{
public:
	PresetPanel(PresetManager &pm) : presetManager(pm),
									 listBoxModel(pm, listBox),
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

		// pass clicks through
		setInterceptsMouseClicks(false, true);

		listBox.setColour(ListBox::ColourIds::backgroundColourId, Colour::fromRGB(22, 22, 22));
		listBox.setColour(ListBox::ColourIds::textColourId, Colours::white);
		listBox.setColour(ListBox::ColourIds::outlineColourId, Colours::black);
		listBox.setModel(&listBoxModel);
		addAndMakeVisible(listBox);

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
		auto height = 45;
		auto bounds = getLocalBounds().removeFromTop(height).reduced(4).withTrimmedLeft(4).withTrimmedRight(4);

		saveButton.setBounds(bounds.removeFromLeft(height).reduced(4));
		deleteButton.setBounds(bounds.removeFromLeft(height).reduced(4));
		nextPresetButton.setBounds(bounds.removeFromRight(height).reduced(4));
		previousPresetButton.setBounds(bounds.removeFromRight(height).reduced(4));
		presetList.setBounds(bounds);

		listBox.setBounds(getLocalBounds().withTrimmedTop(height).reduced(4));
	}

private:
	void paint(juce::Graphics &g) override
	{
		Path p;
		p.addRoundedRectangle(getLocalBounds().removeFromTop(45).reduced(4).toFloat(), 15.0f);
		g.setColour(juce::Colour::fromRGB(0, 0, 0));
		g.fillPath(p);
	}

	std::unique_ptr<juce::Drawable> makeIcon(const char *iconString)
	{
		auto parsedIconString{XmlDocument::parse(String(iconString))};
		jassert(parsedIconString != nullptr);
		auto drawableLogoString = Drawable::createFromSVG(*parsedIconString);
		jassert(drawableLogoString != nullptr);

		return drawableLogoString;
	}

	void buttonClicked(Button *button) override
	{
		if (button == &saveButton)
		{
			fileChooser = std::make_unique<FileChooser>(
				"Save Preset",
				PresetManager::defaultDirectory,
				"*." + PresetManager::extension);
			fileChooser->launchAsync(FileBrowserComponent::saveMode, [&](const FileChooser &chooser)
									 {
					const auto resultFile = chooser.getResult();
					presetManager.savePreset(resultFile.getFileNameWithoutExtension());
					loadPresetList(); });
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
	void comboBoxChanged(ComboBox *comboBoxThatHasChanged) override
	{
		if (comboBoxThatHasChanged == &presetList)
		{
			presetManager.loadPreset(presetList.getItemText(presetList.getSelectedItemIndex()));
		}
	}

	void setupButton(Button &button, const String &buttonText)
	{
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

	ListBox listBox;

	PresetManager &presetManager;
	DrawableButton saveButton, deleteButton, previousPresetButton, nextPresetButton;
	ComboBox presetList;
	std::unique_ptr<FileChooser> fileChooser;

	ComboBoxLookAndFeel comboBoxLAF;

	CustomListBoxModel listBoxModel;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetPanel)
};