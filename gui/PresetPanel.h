#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

#include "LookAndFeel/ComboBoxLookAndFeel.h"
#include "BurgerAlertWindow.h"

std::unique_ptr<juce::Drawable> makeIcon(const char *iconString)
{
	auto parsedIconString{XmlDocument::parse(String(iconString))};
	jassert(parsedIconString != nullptr);
	auto drawableLogoString = Drawable::createFromSVG(*parsedIconString);
	jassert(drawableLogoString != nullptr);

	return drawableLogoString;
}

class CustomListBoxModel : public ListBoxModel
{
public:
	CustomListBoxModel(PresetManager &pm, ListBox &lb) : presetManager(pm), listBox(lb)
	{
		filesFolders = presetManager.getPresetFileHierarchy();

		for (auto &file : filesFolders)
		{
			isCollapsed[file.getRelativePathFrom(pm.defaultDirectory)] = true;
		}

		refreshFilesToRender();

		auto folderClosedIconString = R"svgDELIM(
			<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#ffffff" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="lucide lucide-folder-closed"><path d="M20 20a2 2 0 0 0 2-2V8a2 2 0 0 0-2-2h-7.9a2 2 0 0 1-1.69-.9L9.6 3.9A2 2 0 0 0 7.93 3H4a2 2 0 0 0-2 2v13a2 2 0 0 0 2 2Z"/><path d="M2 10h20"/></svg>
		)svgDELIM";

		auto folderOpenIconString = R"svgDELIM(
			<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#ffffff" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="lucide lucide-folder-open"><path d="m6 14 1.5-2.9A2 2 0 0 1 9.24 10H20a2 2 0 0 1 1.94 2.5l-1.54 6a2 2 0 0 1-1.95 1.5H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h3.9a2 2 0 0 1 1.69.9l.81 1.2a2 2 0 0 0 1.67.9H18a2 2 0 0 1 2 2v2"/></svg>
		)svgDELIM";

		folderClosedIcon = makeIcon(folderClosedIconString);
		folderOpenIcon = makeIcon(folderOpenIconString);

		listBox.setRowHeight(36);
		folderClosedIcon->setVisible(true);
		folderOpenIcon->setVisible(true);
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
					if (isCollapsed[currentFile] || tries++ > 40)
						break;

					current = current.getParentDirectory();
				}

				if (!isCollapsed[current.getRelativePathFrom(presetManager.defaultDirectory)])
				{
					filesToRender.add(file);
				}
			}
		}
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
		

		if (rowIsSelected && !isDir)
			g.fillAll(Colour::fromRGB(33, 33, 33));

		auto depth = row.getRelativePathFrom(presetManager.defaultDirectory).retainCharacters("/\\").length();

		if (isDir)
		{
			bool collapsed = isCollapsed[row.getRelativePathFrom(presetManager.defaultDirectory)];
			g.fillAll(Colour::fromRGB(22, 22, 22));

			g.setColour(Colours::white);

			auto drawArea = Rectangle<float>(5 + depth * 30, 0, height, height).reduced(12).toFloat();

			if (collapsed) {
				folderClosedIcon->drawWithin(g, drawArea, RectanglePlacement::centred, 1.0f);
			} else {
				folderOpenIcon->drawWithin(g, drawArea, RectanglePlacement::centred, 1.0f);
			}
			

		}

		auto extraRoom = 0;

		if (isDir)
			extraRoom = 35;

		g.setColour(LookAndFeel::getDefaultLookAndFeel().findColour(Label::textColourId));
		g.setFont((float)height * 0.4f); // TODO: add font to this

		g.drawText(row.getFileNameWithoutExtension(),
				   15 + depth * 30 + extraRoom, 0, width, height,
				   Justification::centredLeft, true);
	}

	void listBoxItemClicked(int row, const MouseEvent &mouseEvent) override
	{
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
			DBG(item.getFullPathName());
			presetManager.loadPreset(item);

			for (auto listener : singleClickListener)
			{
				listener.operator()(item.getParentDirectory().getFileNameWithoutExtension() + " - " + item.getFileNameWithoutExtension());
			}
		}
	}

	void listBoxItemDoubleClicked(int row, const MouseEvent &mouseEvent) override
	{
		auto item = filesToRender[row];
		if (item.isDirectory())
			return;

		for (auto listener : doubleClickListener)
		{
			listener.operator()(item.getParentDirectory().getFileNameWithoutExtension() + " - " + item.getFileNameWithoutExtension());
		}
	}

	// function to add lambdas that take a string argument
	void addSingleClickListener(std::function<void(String)> listener)
	{
		singleClickListener.push_back(listener);
	}

	void addDoubleClickListener(std::function<void(String)> listener)
	{
		doubleClickListener.push_back(listener);
	}

	void updateContent()
	{
		filesFolders = presetManager.getPresetFileHierarchy();
	}

private:
	PresetManager &presetManager;
	ListBox &listBox;

	std::vector<std::function<void(String)>> singleClickListener;
	std::vector<std::function<void(String)>> doubleClickListener;

	juce::Array<File> filesFolders;
	juce::Array<File> filesToRender;

	std::map<String, bool> isCollapsed;

	std::unique_ptr<juce::Drawable> folderClosedIcon;
	std::unique_ptr<juce::Drawable> folderOpenIcon;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomListBoxModel)
};

class PresetPanel : public Component, Button::Listener
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

		listBox.setColour(ListBox::ColourIds::backgroundColourId, Colours::black);
		listBox.setColour(ListBox::ColourIds::textColourId, Colours::white);
		listBox.setColour(ListBox::ColourIds::outlineColourId, Colours::black);
		listBox.setModel(&listBoxModel);
		addAndMakeVisible(listBox);

		listBoxModel.addSingleClickListener(std::function<void(String)>([&](String presetName)
																		{ currentPresetLabel.setButtonText(presetName); }));

		listBoxModel.addDoubleClickListener(std::function<void(String)>([&](String presetName)
																		{
			showPresetsList = false;
			listBox.setVisible(showPresetsList);
			resized(); }));

		listBox.setVisible(showPresetsList);

		currentPresetLabel.setLookAndFeel(&comboBoxLAF);
		currentPresetLabel.setColour(ComboBox::backgroundColourId, Colours::black);
		currentPresetLabel.setColour(TextButton::ColourIds::buttonColourId, Colours::black);
		// todo: draw chevron on the left to indicate clickability

		currentPresetLabel.setMouseCursor(MouseCursor::PointingHandCursor);
		addAndMakeVisible(currentPresetLabel);
		currentPresetLabel.addListener(this);

		currentPresetLabel.onClick = [this]
		{
			showPresetsList = !showPresetsList;
			listBox.setVisible(showPresetsList);
			resized();
		};

		loadPresetList();
	}

	~PresetPanel()
	{
		saveButton.removeListener(this);
		deleteButton.removeListener(this);
		previousPresetButton.removeListener(this);
		nextPresetButton.removeListener(this);
	}

	void resized() override
	{
		auto height = 45;
		auto bounds = getLocalBounds().removeFromTop(height).reduced(4).withTrimmedLeft(4).withTrimmedRight(4);

		saveButton.setBounds(bounds.removeFromLeft(height).reduced(4));
		deleteButton.setBounds(bounds.removeFromLeft(height).reduced(4));
		nextPresetButton.setBounds(bounds.removeFromRight(height).reduced(4));
		previousPresetButton.setBounds(bounds.removeFromRight(height).reduced(4));
		currentPresetLabel.setBounds(bounds);

		if (showPresetsList)
			listBox.setBounds(getLocalBounds().withTrimmedTop(height).reduced(4));
	}

	void updatePresetLabel(juce::String &text)
	{
		currentPresetLabel.setButtonText(text);
	}

private:
	void paint(juce::Graphics &g) override
	{
		Path p;
		p.addRoundedRectangle(getLocalBounds().removeFromTop(45).reduced(4).toFloat(), 15.0f);
		g.setColour(juce::Colour::fromRGB(0, 0, 0));
		g.fillPath(p);

		// Rectangle<int> arrowZone(5 + 45 * 2, 0, 20, 45);
        // Path path;
        // path.startNewSubPath((float)arrowZone.getX() + 4.0f, (float)arrowZone.getCentreY() - 2.0f);
        // path.lineTo((float)arrowZone.getCentreX(), (float)arrowZone.getCentreY() + 3.0f);
        // path.lineTo((float)arrowZone.getRight() - 4.0f, (float)arrowZone.getCentreY() - 2.0f);

        // g.setColour(juce::Colours::white);
        // g.strokePath(path, PathStrokeType(2.0f, juce::PathStrokeType::JointStyle::curved, juce::PathStrokeType::EndCapStyle::rounded));
	}

	void buttonClicked(Button *button) override
	{
		if (button == &saveButton)
		{
			auto alertWindow = new BurgerAlertWindow("Save Preset", "Enter a name for your new preset: ", MessageBoxIconType::NoIcon);

			alertWindow->enterModalState(true, juce::ModalCallbackFunction::create([this, alertWindow](int result)
																				   {
				if (result == 1) {
					auto name = alertWindow->getTextEditor("presetName")->getText();
					presetManager.savePreset(name);
					loadPresetList();
				}

				delete alertWindow; }));
		}
		if (button == &previousPresetButton)
		{
			const auto newPreset = presetManager.loadPreviousPreset();
			currentPresetLabel.setButtonText(newPreset.getParentDirectory().getFileNameWithoutExtension() + " - " + newPreset.getFileNameWithoutExtension());
		}
		if (button == &nextPresetButton)
		{
			const auto newPreset = presetManager.loadNextPreset();
			currentPresetLabel.setButtonText(newPreset.getParentDirectory().getFileNameWithoutExtension() + " - " + newPreset.getFileNameWithoutExtension());
		}
		if (button == &deleteButton)
		{
			presetManager.deletePreset(presetManager.getCurrentPreset());
			loadPresetList();

			// auto alertWindow2 = new BurgerAlertWindow("Delete Preset", "Are you sure you want to delete this preset? ", MessageBoxIconType::NoIcon);
			// alertWindow2->addButton("Delete", 1, KeyPress(KeyPress::returnKey, 0, 0));
			// alertWindow2->addButton("Cancel", 0, KeyPress(KeyPress::escapeKey, 0, 0));
			// alertWindow2->enterModalState(true, nullptr, true);
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

		const auto allPresets = presetManager.getAllPresets();
		const auto currentPreset = presetManager.getCurrentPreset();

		juce::StringArray presets;
		for (auto &file : allPresets)
		{
			presets.add(file.getFileNameWithoutExtension());
		}

		currentPresetLabel.setButtonText("Presets");
	}

	ListBox listBox;

	PresetManager &presetManager;
	DrawableButton saveButton, deleteButton, previousPresetButton, nextPresetButton;

	bool showPresetsList = false;

	TextButton currentPresetLabel;

	std::unique_ptr<FileChooser> fileChooser;

	ComboBoxLookAndFeel comboBoxLAF;

	CustomListBoxModel listBoxModel;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetPanel)
};