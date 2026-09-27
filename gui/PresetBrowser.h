#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

#include "BinaryData.h"
#include "../service/PresetManager.h"
#include "LookAndFeel/Theme.h"
#include "Modules/Panel.h"

inline std::unique_ptr<juce::Drawable> makeIcon(const char *iconString)
{
	auto parsedIconString{juce::XmlDocument::parse(juce::String(iconString))};
	jassert(parsedIconString != nullptr);
	auto drawableLogoString = juce::Drawable::createFromSVG(*parsedIconString);
	jassert(drawableLogoString != nullptr);

	return drawableLogoString;
}

// lucide icons, drawn in white for IconButton to recolour
namespace PresetIcons
{
	static constexpr const char *save = R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" fill="none" stroke="white" stroke-linecap="round" stroke-linejoin="round" stroke-width="2"><path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z"/><path d="M17 21v-8H7v8M7 3v5h8"/></svg>)svg";
	static constexpr const char *remove = R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" fill="none" stroke="white" stroke-linecap="round" stroke-linejoin="round" stroke-width="2"><path d="M3 6h18M19 6v14c0 1-1 2-2 2H7c-1 0-2-1-2-2V6M8 6V4c0-1 1-2 2-2h4c1 0 2 1 2 2v2M10 11v6M14 11v6"/></svg>)svg";
	static constexpr const char *rename = R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21.174 6.812a1 1 0 0 0-3.986-3.987L3.842 16.174a2 2 0 0 0-.5.83l-1.321 4.352a.5.5 0 0 0 .623.622l4.353-1.32a2 2 0 0 0 .83-.497z"/><path d="m15 5 4 4"/></svg>)svg";
	static constexpr const char *folder = R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M20 20a2 2 0 0 0 2-2V8a2 2 0 0 0-2-2h-7.9a2 2 0 0 1-1.69-.9L9.6 3.9A2 2 0 0 0 7.93 3H4a2 2 0 0 0-2 2v13a2 2 0 0 0 2 2Z"/><path d="M2 10h20"/></svg>)svg";
	static constexpr const char *previous = R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="m15 18-6-6 6-6"/></svg>)svg";
	static constexpr const char *next = R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="m9 18 6-6-6-6"/></svg>)svg";
	static constexpr const char *close = R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 30 30" fill="none" stroke="white" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M18 6 6 18"/><path d="m6 6 12 12"/></svg>)svg";
}

// a white icon, redrawn in the presets' text colour whenever the theme changes
class IconButton : public juce::DrawableButton
{
public:
	IconButton(const juce::String &tooltip, const char *svg) : juce::DrawableButton(tooltip, ImageOnButtonBackground), icon(makeIcon(svg))
	{
		setTooltip(tooltip);
		setMouseCursor(juce::MouseCursor::PointingHandCursor);
		lookAndFeelChanged();
	}

	void lookAndFeelChanged() override
	{
		auto themed = icon->createCopy();
		themed->replaceColour(juce::Colours::white, theme().textPresets);
		setImages(themed.get());
		setColour(juce::TextButton::buttonColourId, theme().box);
		setColour(juce::TextButton::buttonOnColourId, theme().box);
	}

private:
	std::unique_ptr<juce::Drawable> icon;
};

/*	The preset view: the preset folder's folders on the left, the chosen folder's presets beside them, and on the right what
	the chosen preset says about itself, with the tools for presets under it. Folders are one level deep. */
class PresetBrowser : public juce::Component
{
public:
	std::function<void(const juce::File &)> onPresetClicked, onPresetDoubleClicked;

	IconButton saveButton{"Save the current sound as a preset", PresetIcons::save},
		renameButton{"Rename the selected preset", PresetIcons::rename},
		deleteButton{"Delete the selected preset", PresetIcons::remove},
		folderButton{"Open the preset folder", PresetIcons::folder};

	explicit PresetBrowser(Preset::PresetManager &pm) : presetManager(pm)
	{
		folders.paintRow = [this](juce::Graphics &g, int row, int width, int height, bool isSelected)
		{
			const auto &folder = folders.files[row];
			paintRow(g, folder == presetManager.getPresetDirectory() ? juce::String("Presets") : folder.getFileName(), {}, width, height, isSelected);
		};
		folders.onClick = [this](int row) { showFolder(folders.files[row]); };

		presets.paintRow = [this](juce::Graphics &g, int row, int width, int height, bool isSelected)
		{
			paintRow(g, presets.files[row].getFileNameWithoutExtension(), info[row].getAuthor(), width, height, isSelected);
		};
		presets.onClick = [this](int row)
		{
			selected = info[row];
			repaint();
			onPresetClicked(selected.getFile());
		};
		presets.onDoubleClick = [this](int row) { onPresetDoubleClicked(presets.files[row]); };

		for (auto *list : {&folderList, &presetList})
		{
			list->setRowHeight(rowHeight);
			list->setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
			list->setOutlineThickness(0);
			addAndMakeVisible(list);
		}

		folderList.setModel(&folders);
		presetList.setModel(&presets);

		for (auto *button : {&saveButton, &renameButton, &deleteButton, &folderButton})
			addAndMakeVisible(button);

		quicksand.setHeight(18.0f);
	}

	juce::File getSelected() const { return selected.getFile(); }

	// the folders and presets afresh from disk, keeping to the current preset's folder and the current preset
	void refresh()
	{
		const auto current = presetManager.getCurrentPreset();
		folders.files = presetManager.getFolders();
		folderList.updateContent();

		auto folder = current.existsAsFile() ? current.getParentDirectory() : shownFolder;

		if (!folders.files.contains(folder))
			folder = folders.files.isEmpty() ? juce::File() : folders.files.getFirst();

		selected = Preset::PresetFile(current.existsAsFile() ? current : selected.getFile());
		showFolder(folder);
	}

	void lookAndFeelChanged() override
	{
		for (auto *list : {&folderList, &presetList})
		{
			auto &bar = list->getVerticalScrollBar();
			bar.setColour(juce::ScrollBar::thumbColourId, theme().scrollbar);
			bar.setColour(juce::ScrollBar::trackColourId, theme().scrollbar);
			bar.setColour(juce::ScrollBar::backgroundColourId, theme().box);
		}
	}

	void paint(juce::Graphics &g) override
	{
		const auto box = getLocalBounds().reduced(Panel::boxInset).toFloat();
		g.setColour(theme().box);
		g.fillRoundedRectangle(box, Panel::boxCornerSize);

		g.setColour(theme().divider);
		for (const auto x : {folderList.getRight() + gap / 2, presetList.getRight() + gap / 2})
			g.drawVerticalLine(x, box.getY() + gap, box.getBottom() - gap);

		g.setFont(quicksand.withHeight(13.0f));
		g.setColour(theme().textPresetsAuthor);
		g.drawText("FOLDERS", folderList.getBounds().withY(box.toNearestInt().getY() + gap).withHeight(headingHeight), juce::Justification::centredLeft);
		g.drawText("PRESETS", presetList.getBounds().withY(box.toNearestInt().getY() + gap).withHeight(headingHeight), juce::Justification::centredLeft);

		// what the chosen preset says about itself
		auto area = infoArea;
		g.setColour(theme().textPresets);
		g.setFont(quicksand.withHeight(22.0f));
		g.drawFittedText(selected.getFile().getFileNameWithoutExtension(), area.removeFromTop(30), juce::Justification::centredLeft, 1);

		g.setFont(quicksand.withHeight(15.0f));
		g.setColour(theme().textPresetsAuthor);
		const auto author = selected.getAuthor();
		g.drawFittedText(author.isEmpty() ? juce::String() : "by " + author, area.removeFromTop(22), juce::Justification::centredLeft, 1);

		area.removeFromTop(8);
		g.setColour(theme().textPresets);
		g.drawFittedText(selected.getDescription(), area, juce::Justification::topLeft, area.getHeight() / 17, 1.0f);
	}

	void resized() override
	{
		auto area = getLocalBounds().reduced(Panel::boxInset + gap);

		auto infoColumn = area.removeFromRight(area.getWidth() * 3 / 10);
		area.removeFromRight(gap);
		auto folderColumn = area.removeFromLeft(area.getWidth() / 3);
		area.removeFromLeft(gap);

		folderList.setBounds(folderColumn.withTrimmedTop(headingHeight));
		presetList.setBounds(area.withTrimmedTop(headingHeight));

		auto tools = infoColumn.removeFromBottom(toolHeight);
		const auto toolWidth = tools.getWidth() / 4;
		for (auto *button : {&saveButton, &renameButton, &deleteButton, &folderButton})
			button->setBounds(tools.removeFromLeft(toolWidth).reduced(6));

		infoArea = infoColumn.withTrimmedBottom(gap);
	}

private:
	struct FileList : juce::ListBoxModel
	{
		juce::Array<juce::File> files;
		std::function<void(juce::Graphics &, int, int, int, bool)> paintRow;
		std::function<void(int)> onClick, onDoubleClick;

		int getNumRows() override { return files.size(); }

		void paintListBoxItem(int row, juce::Graphics &g, int width, int height, bool isSelected) override
		{
			if (juce::isPositiveAndBelow(row, files.size()))
				paintRow(g, row, width, height, isSelected);
		}

		void listBoxItemClicked(int row, const juce::MouseEvent &) override
		{
			if (juce::isPositiveAndBelow(row, files.size()))
				onClick(row);
		}

		void listBoxItemDoubleClicked(int row, const juce::MouseEvent &) override
		{
			if (onDoubleClick != nullptr && juce::isPositiveAndBelow(row, files.size()))
				onDoubleClick(row);
		}

		juce::MouseCursor getMouseCursorForRow(int) override { return juce::MouseCursor::PointingHandCursor; }
	};

	void paintRow(juce::Graphics &g, const juce::String &name, const juce::String &author, int width, int height, bool isSelected)
	{
		if (isSelected)
		{
			g.setColour(theme().rowSelected);
			g.fillRoundedRectangle(juce::Rectangle<int>(width, height).reduced(0, 2).toFloat(), 6.0f);
		}

		g.setFont(quicksand);
		g.setColour(theme().textPresets);
		g.drawText(name, 10, 0, width - 20, height, juce::Justification::centredLeft, true);

		g.setColour(theme().textPresetsAuthor);
		g.drawText(author, 10, 0, width - 20, height, juce::Justification::centredRight, true);
	}

	// the folder's presets, with what each says about itself read once here rather than on every paint
	void showFolder(const juce::File &folder)
	{
		shownFolder = folder;
		folderList.selectRow(folders.files.indexOf(folder), true, true);

		presets.files = folder.isDirectory() ? presetManager.getPresetsIn(folder) : juce::Array<juce::File>();
		info.clearQuick();
		for (const auto &file : presets.files)
			info.add(Preset::PresetFile(file));

		presetList.updateContent();
		presetList.selectRow(presets.files.indexOf(selected.getFile()), true, true);
		repaint();
	}

	static constexpr int rowHeight = 32, gap = 10, headingHeight = 22, toolHeight = 44;

	Preset::PresetManager &presetManager;

	FileList folders, presets;
	juce::ListBox folderList, presetList;
	juce::Array<Preset::PresetFile> info;

	juce::File shownFolder;
	Preset::PresetFile selected;
	juce::Rectangle<int> infoArea;

	const juce::Typeface::Ptr quicksandTypeface = juce::Typeface::createSystemTypefaceFor(BinaryData::QuicksandBold_ttf, BinaryData::QuicksandBold_ttfSize);
	juce::Font quicksand{juce::FontOptions(quicksandTypeface)};

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowser)
};
