#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

#include "BurgerAlert.h"

std::unique_ptr<juce::Drawable> makeIcon(const char *iconString)
{
	auto parsedIconString{juce::XmlDocument::parse(juce::String(iconString))};
	jassert(parsedIconString != nullptr);
	auto drawableLogoString = juce::Drawable::createFromSVG(*parsedIconString);
	jassert(drawableLogoString != nullptr);

	return drawableLogoString;
}

const std::function<void(std::string)> errorAlertCallback = [](std::string result)
{
	DBG(result);
	auto errorAlert = new BurgerAlert("Error", result, juce::AlertWindow::AlertIconType::WarningIcon);

	errorAlert->createPresetWarning();
	errorAlert->enterModalState(true, nullptr, true);
};

class CustomListBoxModel : public juce::ListBoxModel
{
public:
	CustomListBoxModel(Preset::PresetManager &pm, juce::ListBox &lb) : presetManager(pm), listBox(lb)
	{
		auto folderClosedIconString = R"svgDELIM(
			<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#ffffff" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="lucide lucide-folder-closed"><path d="M20 20a2 2 0 0 0 2-2V8a2 2 0 0 0-2-2h-7.9a2 2 0 0 1-1.69-.9L9.6 3.9A2 2 0 0 0 7.93 3H4a2 2 0 0 0-2 2v13a2 2 0 0 0 2 2Z"/><path d="M2 10h20"/></svg>
		)svgDELIM";

		auto folderOpenIconString = R"svgDELIM(
			<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#ffffff" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="lucide lucide-folder-open"><path d="m6 14 1.5-2.9A2 2 0 0 1 9.24 10H20a2 2 0 0 1 1.94 2.5l-1.54 6a2 2 0 0 1-1.95 1.5H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h3.9a2 2 0 0 1 1.69.9l.81 1.2a2 2 0 0 0 1.67.9H18a2 2 0 0 1 2 2v2"/></svg>
		)svgDELIM";

		folderClosedIcon = makeIcon(folderClosedIconString);
		folderOpenIcon = makeIcon(folderOpenIconString);

		listBox.setRowHeight(36);

		// font = quicksandFont.get();
		quicksandFont.setHeight(20.0f);
	}

    const juce::Typeface::Ptr quicksandTypeface = juce::Typeface::createSystemTypefaceFor(BinaryData::QuicksandBold_ttf, BinaryData::QuicksandBold_ttfSize);
    juce::Font quicksandFont = juce::Font(juce::FontOptions(quicksandTypeface));

	void initFiles(bool ignoreCollapsedStates = false)
	{
		updateContent();

		if (!ignoreCollapsedStates)
		{
			for (auto &file : *filesFolders)
			{
				isCollapsed[file->getFile().getRelativePathFrom(presetManager.getPresetDirectory())] = true;
			}
		}

		refreshFilesToRender();
	}

	void refreshFilesToRender()
	{
		filesToRender.clear();

		auto &collection = *filesFolders;

		for (auto &file : collection)
		{
			auto relativePath = file->getFile().getRelativePathFrom(presetManager.getPresetDirectory());
			auto depth = relativePath.retainCharacters("/\\").length();

			if (depth == 0)
			{
				filesToRender.add(file->getFile());
				continue;
			}

			auto parent = file->getFile().getParentDirectory();
			juce::File current = parent;

			int tries = 0;

			while (current != presetManager.getPresetDirectory())
			{
				auto currentFile = current.getRelativePathFrom(presetManager.getPresetDirectory());
				if (isCollapsed[currentFile] || tries++ > 40)
					break;

				current = current.getParentDirectory();
			}

			if (!isCollapsed[current.getRelativePathFrom(presetManager.getPresetDirectory())])
			{
				filesToRender.add(file->getFile());
			}
		}
	}

	int getNumRows() override
	{
		return filesToRender.size();
	}

	juce::MouseCursor getMouseCursorForRow(int row) override
	{
		return juce::MouseCursor::PointingHandCursor;
	}

	void paintListBoxItem(int rowNumber, juce::Graphics &g,
						  int width, int height, bool rowIsSelected) override
	{
		if (filesToRender.size() == 0)
			return;

		auto row = filesToRender[rowNumber];

		auto isDir = row.getFile().isDirectory();

		if (rowIsSelected && !isDir)
		{
			g.fillAll(juce::Colour::fromRGB(33, 33, 33));
		}

		auto depth = row.getFile().getRelativePathFrom(presetManager.getPresetDirectory()).retainCharacters("/\\").length();

		if (isDir)
		{
			bool collapsed = isCollapsed[row.getFile().getRelativePathFrom(presetManager.getPresetDirectory())];
			int delta = 0;

			// Path p;
			// p.addRoundedRectangle(5 + depth * 30, 0, width - (depth * 30 + delta * 4), height - 2 * delta, 15.0f);
			g.setColour(juce::Colour::fromRGB(44, 44, 44));
			// g.fillPath(p);
			g.drawLine(5 + depth * 30, height, width, height, 2);

			// g.fillAll(juce::Colour::fromRGB(22, 22, 22));

			g.setColour(juce::Colours::white);

			auto drawArea = juce::Rectangle<float>(5 + depth * 30, 0, height, height).reduced(12).toFloat();

			if (collapsed)
			{
				folderClosedIcon->drawWithin(g, drawArea, juce::RectanglePlacement::centred, 1.0f);
			}
			else
			{
				folderOpenIcon->drawWithin(g, drawArea, juce::RectanglePlacement::centred, 1.0f);
			}
		}

		auto extraRoom = 0;

		if (isDir)
			extraRoom = 35;

		g.setColour(juce::LookAndFeel::getDefaultLookAndFeel().findColour(juce::Label::textColourId));
		g.setFont(quicksandFont);

		auto presetName = row.getFile().getFileNameWithoutExtension();

		if (presetName.length() > 20)
		{
			presetName = presetName.substring(0, 20) + "...";
		}

		g.drawText(presetName,
				   15 + depth * 30 + extraRoom, 0, width, height,
				   juce::Justification::centredLeft, true);

		g.setColour(juce::Colour::fromRGB(100, 100, 100));
		g.setFont(quicksandFont);


		auto authorName = row.getAuthor();

		if (authorName.length() > 15) {
			authorName = authorName.substring(0, 15) + "...";
		}

		g.drawText(authorName,
				   0, 0, width - 20, height,
				   juce::Justification::centredRight, true);
	}

	void listBoxItemClicked(int row, const juce::MouseEvent &mouseEvent) override
	{
		auto item = filesToRender[row];

		auto isDir = item.getFile().isDirectory();

		if (isDir)
		{
			auto relativePath = item.getFile().getRelativePathFrom(presetManager.getPresetDirectory());
			isCollapsed[relativePath] = !isCollapsed[relativePath];
			refreshFilesToRender();
			listBox.updateContent();
		}
		else
		{
			DBG(item.getFile().getFullPathName());
			presetManager.loadPreset(item.getFile(), errorAlertCallback);

			for (auto listener : singleClickListener)
			{
				listener.operator()(item.getFile().getParentDirectory().getFileNameWithoutExtension() + " - " + item.getFile().getFileNameWithoutExtension());
			}
		}
	}

	void listBoxItemDoubleClicked(int row, const juce::MouseEvent &mouseEvent) override
	{
		auto item = filesToRender[row];
		if (item.getFile().isDirectory())
			return;

		for (auto listener : doubleClickListener)
		{
			listener.operator()(item.getFile().getParentDirectory().getFileNameWithoutExtension() + " - " + item.getFile().getFileNameWithoutExtension());
		}
	}

	// function to add lambdas that take a string argument
	void addSingleClickListener(std::function<void(juce::String)> listener)
	{
		singleClickListener.push_back(listener);
	}

	void addDoubleClickListener(std::function<void(juce::String)> listener)
	{
		doubleClickListener.push_back(listener);
	}

	void updateContent()
	{
		filesFolders = presetManager.getPresetFileHierarchy();
	}

private:
	Preset::PresetManager &presetManager;
	juce::ListBox &listBox;

	std::vector<std::function<void(juce::String)>> singleClickListener;
	std::vector<std::function<void(juce::String)>> doubleClickListener;

	std::shared_ptr<juce::OwnedArray<Preset::PresetFile>> filesFolders = nullptr;
	juce::Array<Preset::PresetFile> filesToRender;

	std::map<juce::String, bool> isCollapsed;

	std::unique_ptr<juce::Drawable> folderClosedIcon;
	std::unique_ptr<juce::Drawable> folderOpenIcon;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomListBoxModel)
};

class CustomPresetListBox : public juce::ListBox
{
public:
	CustomPresetListBox()
	{

		setColour(juce::ListBox::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
		setColour(juce::ListBox::ColourIds::textColourId, juce::Colours::white);
		setColour(juce::ListBox::ColourIds::outlineColourId, juce::Colours::black);

		setOpaque(false);

		auto &vertScroll = this->getVerticalScrollBar();

		vertScroll.setColour(juce::ScrollBar::ColourIds::thumbColourId, Palette::colours[3]);
		vertScroll.setColour(juce::ScrollBar::ColourIds::trackColourId, Palette::colours[3]);
		vertScroll.setColour(juce::ScrollBar::ColourIds::backgroundColourId, juce::Colours::black);
	}

	void paint(juce::Graphics &g) override
	{
		// if (this->hasDoneInitialUpdate) {
		// 	updateContent()
		// }

		juce::Path p;
		p.addRoundedRectangle(getLocalBounds().reduced(4).withTrimmedTop(-4).toFloat(), 15.0f);
		g.setColour(juce::Colour::fromRGB(0, 0, 0));
		g.fillPath(p);
	}

private:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomPresetListBox)
};

class PresetPanel : public juce::Component, juce::Button::Listener
{
public:
	PresetPanel(Preset::PresetManager &pm) : presetManager(pm),
											 listBoxModel(pm, listBox),
											 saveButton("Save", juce::DrawableButton::ImageOnButtonBackground),
											 deleteButton("Delete", juce::DrawableButton::ImageOnButtonBackground),
											 previousPresetButton("Previous", juce::DrawableButton::ImageOnButtonBackground),
											 nextPresetButton("Next", juce::DrawableButton::ImageOnButtonBackground),
											 closeButton("Close", juce::DrawableButton::ImageOnButtonBackground),
											 openPresetFolderButton("Open Folder", juce::DrawableButton::ImageOnButtonBackground)
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

		auto closeIcon = makeIcon(R"svgDelim(
			<svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 30 30" fill="none" stroke="white" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="lucide lucide-x"><path d="M18 6 6 18"/><path d="m6 6 12 12"/></svg>
		)svgDelim");
		closeButton.setImages(closeIcon.get());

		auto folderClosedIconString = makeIcon(R"svgDELIM(
			<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#ffffff" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="lucide lucide-folder-closed"><path d="M20 20a2 2 0 0 0 2-2V8a2 2 0 0 0-2-2h-7.9a2 2 0 0 1-1.69-.9L9.6 3.9A2 2 0 0 0 7.93 3H4a2 2 0 0 0-2 2v13a2 2 0 0 0 2 2Z"/><path d="M2 10h20"/></svg>
		)svgDELIM");

		openPresetFolderButton.setImages(folderClosedIconString.get());

		setupButton(saveButton, "Save");
		setupButton(deleteButton, "Delete");
		setupButton(previousPresetButton, "<");
		setupButton(nextPresetButton, ">");
		setupButton(closeButton, "X");
		setupButton(openPresetFolderButton, "Open Folder");

		closeButton.setVisible(false);
		closeButton.onClick = [this]
		{
			showPresetsList = false;
			listBox.setVisible(showPresetsList);
			closeButton.setVisible(false);
			resized();
		};

		listBox.setModel(&listBoxModel);
		addAndMakeVisible(listBox);

		listBoxModel.addSingleClickListener(std::function<void(juce::String)>([&](juce::String presetName)
																		{ currentPresetLabel.setButtonText(presetName); }));

		listBoxModel.addDoubleClickListener(std::function<void(juce::String)>([&](juce::String presetName)
																		{
			showPresetsList = false;
			listBox.setVisible(showPresetsList);
			closeButton.setVisible(false);
			resized(); }));

		listBox.setVisible(showPresetsList);

		openPresetFolderButton.onClick = [this, &pm]
		{
			pm.getPresetDirectory().startAsProcess();
		};

		// currentPresetLabel.setLookAndFeel(&comboBoxLAF);
		currentPresetLabel.setColour(juce::ComboBox::backgroundColourId, juce::Colours::black);
		currentPresetLabel.setColour(juce::TextButton::ColourIds::buttonColourId, juce::Colours::black);

		currentPresetLabel.setMouseCursor(juce::MouseCursor::PointingHandCursor);
		addAndMakeVisible(currentPresetLabel);
		currentPresetLabel.addListener(this);

		currentPresetLabel.onClick = [this]
		{
			showPresetsList = !showPresetsList;
			listBox.setVisible(showPresetsList);
			closeButton.setVisible(showPresetsList);
			resized();
		};

		// pass clicks through
		setInterceptsMouseClicks(false, true);

		// MessageManager::callAsync([this]
		// 						  {
			loadPresetList();
			this->listBoxModel.initFiles();
			this->listBox.updateContent();
			this->listBox.repaint(); 
			// });

		currentPresetLabel.setButtonText("Hamburger");
	}

	~PresetPanel() override {
		currentPresetLabel.setLookAndFeel(nullptr);
		
	};

	void resized() override
	{
		auto height = 45;
		auto bounds = getLocalBounds().removeFromTop(height).reduced(4).withTrimmedLeft(4).withTrimmedRight(4);

		saveButton.setBounds(bounds.removeFromLeft(height).reduced(4));
		deleteButton.setBounds(bounds.removeFromLeft(height).reduced(4));
		openPresetFolderButton.setBounds(bounds.removeFromLeft(height).reduced(4));

		nextPresetButton.setBounds(bounds.removeFromRight(height).reduced(4));
		previousPresetButton.setBounds(bounds.removeFromRight(height).reduced(4));

		// if (showPresetsList)
		closeButton.setBounds(bounds.removeFromRight(height).reduced(8));

		currentPresetLabel.setBounds(bounds.reduced(4));

		auto presetListBounds = getLocalBounds().withTrimmedTop(height);

		presetListBounds.removeFromLeft(getWidth() / 4);
		presetListBounds.removeFromRight(getWidth() / 4);
		presetListBounds.removeFromBottom(presetListBounds.getHeight() / 4);
		presetListBounds.reduce(4, 4);

		// if (showPresetsList)
		listBox.setBounds(presetListBounds);
		DBG(listBox.getX());
	}

	void updatePresetLabel(juce::String &text)
	{
		currentPresetLabel.setButtonText(text);
	}

private:
	void paint(juce::Graphics &g) override
	{
		juce::Path p;
		p.addRoundedRectangle(getLocalBounds().removeFromTop(45).reduced(4).toFloat(), 15.0f);
		g.setColour(juce::Colour::fromRGB(0, 0, 0));
		g.fillPath(p);
	}

	void buttonClicked(juce::Button *button) override
	{
		if (button == &saveButton)
		{
			auto alertWindow = new BurgerAlert("Save Preset", "Enter a name for your new preset: ", juce::MessageBoxIconType::NoIcon);

			auto defaultAuthor = presetManager.getCurrentAuthor();
			if (defaultAuthor.isEmpty())
				defaultAuthor = presetManager.getLastAuthor();
			alertWindow->createPresetSaveAlert(presetManager.getCurrentPresetName(), defaultAuthor);

			alertWindow->enterModalState(true, juce::ModalCallbackFunction::create([this, alertWindow](int result)
																				   {
				if (result == 1) {
					auto presetTextEditor = alertWindow->getTextEditor("presetName");
					auto authorTextEditor = alertWindow->getTextEditor("author");
					
					presetTextEditor->unfocusAllComponents();
					authorTextEditor->unfocusAllComponents();
					// or alternatively:
					// alertWindow->unfocusAllComponents();
					
					auto name = presetTextEditor->getText();
					auto author = authorTextEditor->getText();
					// auto description = alertWindow->getTextEditor("description")->getText();

					auto saveSuccess = presetManager.savePreset(name, author, "", errorAlertCallback);

					if (!saveSuccess) {
						// auto errorAlert = new BurgerAlert("Error", "Preset was unable to be saved (it already exists?)", juce::AlertWindow::AlertIconType::WarningIcon);

						// errorAlert->createPresetWarning();
						// errorAlert->enterModalState(true, nullptr, true);
					} else {
						loadPresetList();
						this->listBoxModel.initFiles();
						this->listBox.repaint();

						this->currentPresetLabel.setButtonText(name);
					}
				}

				delete alertWindow; }));
		}
		if (button == &previousPresetButton)
		{
			const auto newPreset = presetManager.loadPreviousPreset(errorAlertCallback);
			currentPresetLabel.setButtonText(newPreset.getParentDirectory().getFileNameWithoutExtension() + " - " + newPreset.getFileNameWithoutExtension());
		}
		if (button == &nextPresetButton)
		{
			const auto newPreset = presetManager.loadNextPreset(errorAlertCallback);
			currentPresetLabel.setButtonText(newPreset.getParentDirectory().getFileNameWithoutExtension() + " - " + newPreset.getFileNameWithoutExtension());
		}
		if (button == &deleteButton)
		{
			auto alertWindow2 = new BurgerAlert("Delete Preset", "Are you sure you want to delete this preset? ", juce::MessageBoxIconType::NoIcon);
			alertWindow2->createPresetDeleteAlert();
			alertWindow2->enterModalState(true, juce::ModalCallbackFunction::create([this, alertWindow2](int result)
																					{
				if (result == 1)
				{
					presetManager.deletePreset(presetManager.getCurrentPreset(), errorAlertCallback);
					loadPresetList();
					this->listBoxModel.initFiles(true);
					this->listBox.repaint();
				}

				delete alertWindow2; }));
		}
	}

	void setupButton(juce::Button &button, const juce::String &buttonText)
	{
		juce::ignoreUnused(buttonText);
		// button.setButtonText(buttonText);
		button.setMouseCursor(juce::MouseCursor::PointingHandCursor);

		button.setColour(juce::TextButton::ColourIds::buttonOnColourId, juce::Colour::fromRGB(0, 0, 0));
		button.setColour(juce::TextButton::ColourIds::buttonColourId, juce::Colour::fromRGB(0, 0, 0));
		button.setColour(juce::TextButton::ColourIds::textColourOffId, juce::Colour::fromRGB(255, 255, 255));
		button.setColour(juce::ComboBox::outlineColourId, juce::Colour::fromRGB(0, 0, 0));

		addAndMakeVisible(button);
		button.addListener(this);
	}

	void loadPresetList()
	{
		const auto allPresets = presetManager.getAllPresets();

		juce::StringArray presets;
		for (auto &file : allPresets)
		{
			presets.add(file.getFileNameWithoutExtension());
		}
	}

	Preset::PresetManager &
		presetManager;
	juce::DrawableButton saveButton, deleteButton, previousPresetButton, nextPresetButton, closeButton, openPresetFolderButton;

	bool showPresetsList = false;

	juce::TextButton currentPresetLabel;

	// HamburgerLAF comboBoxLAF;

	CustomPresetListBox listBox;
	CustomListBoxModel listBoxModel;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetPanel)
};