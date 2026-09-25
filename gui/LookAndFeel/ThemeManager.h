#pragma once

#include "Theme.h"
#include "BinaryData.h"

/*  Which theme is on screen, shared between every open instance through a SharedResourcePointer. Picking a theme or
    changing a colour broadcasts a change, and each editor repaints itself from theme(). Edits are kept in unsaved.ini
    in the themes folder until they're saved under a name. */
class ThemeManager : public juce::ChangeBroadcaster,
                     private juce::Timer
{
public:
    struct Entry
    {
        juce::String id, label;
    };

    static inline const juce::String defaultId { "builtin:Default" };
    static inline const juce::String unsavedId { "custom:unsaved.ini" };

    static juce::File folder()
    {
        return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                   .getChildFile (JucePlugin_Manufacturer)
                   .getChildFile (JucePlugin_Name)
                   .getChildFile ("themes");
    }

    ~ThemeManager() override { writeUnsavedIfPending(); }

    ThemeInfo info = defaultInfo();

    const juce::String& getSelectedId() const noexcept { return selectedId; }

    // the default first, then any built in ini files, then the themes folder
    juce::Array<Entry> list() const
    {
        juce::Array<Entry> entries;
        entries.add ({ defaultId, labelFor (defaultInfo()) });

        for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
            if (juce::String (BinaryData::originalFilenames[i]).endsWithIgnoreCase (".ini"))
                entries.add (entryFor ("builtin:" + juce::String (BinaryData::originalFilenames[i])));

        auto files = folder().findChildFiles (juce::File::findFiles, false, "*.ini");
        files.sort();

        for (const auto& file : files)
            entries.add (entryFor ("custom:" + file.getFileName()));

        // edits waiting on the debounce haven't written their file yet
        if (selectedId == unsavedId && ! folder().getChildFile ("unsaved.ini").existsAsFile())
            entries.add ({ unsavedId, "Unsaved - " + labelFor (info) });

        return entries;
    }

    // the theme's own background from the themes folder or the built in files, hamburger's own if it has none or it won't parse
    std::unique_ptr<juce::XmlElement> backgroundSvg() const
    {
        if (info.background.isNotEmpty())
        {
            const auto file = folder().getChildFile (info.background);

            if (auto svg = juce::parseXML (file.existsAsFile() ? file.loadFileAsString() : textFor ("builtin:" + info.background)))
                return svg;
        }

        return juce::parseXML (juce::String::createStringFromData (BinaryData::hamburgerbg_svg, BinaryData::hamburgerbg_svgSize));
    }

    bool select (const juce::String& id)
    {
        writeUnsavedIfPending();

        if (id == defaultId)
        {
            theme() = Theme();
            info = defaultInfo();
        }
        else
        {
            const auto text = textFor (id);

            if (text.isEmpty())
                return false;

            theme() = themeFromIni (text, info);
        }

        selectedId = id;
        sendChangeMessage();
        return true;
    }

    // after the customiser changes theme() or info, which makes what's on screen the unsaved theme
    void edited()
    {
        selectedId = unsavedId;
        startTimer (1000);
        sendChangeMessage();
    }

    // as "author - name.ini" in the themes folder, replacing one of the same name
    void save()
    {
        stopTimer();

        folder().createDirectory();

        const auto file = folder().getChildFile (juce::File::createLegalFileName (labelFor (info)) + ".ini");
        file.replaceWithText (themeToIni (theme(), info));

        folder().getChildFile ("unsaved.ini").deleteFile();

        selectedId = "custom:" + file.getFileName();
        sendChangeMessage();
    }

private:
    static ThemeInfo defaultInfo() { return { "Default", "Aviary Audio", "The theme Hamburger comes with", {} }; }

    static juce::String labelFor (const ThemeInfo& themeInfo)
    {
        return themeInfo.author.isEmpty() ? themeInfo.name : themeInfo.author + " - " + themeInfo.name;
    }

    Entry entryFor (const juce::String& id) const
    {
        ThemeInfo fileInfo;
        themeFromIni (textFor (id), fileInfo);

        return { id, (id == unsavedId ? "Unsaved - " : "") + labelFor (fileInfo) };
    }

    static juce::String textFor (const juce::String& id)
    {
        const auto name = id.fromFirstOccurrenceOf (":", false, false);

        if (id.startsWith ("custom:"))
            return folder().getChildFile (name).loadFileAsString();

        for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
        {
            if (name == BinaryData::originalFilenames[i])
            {
                int size = 0;
                const auto* data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], size);
                return juce::String::fromUTF8 (data, size);
            }
        }

        return {};
    }

    void timerCallback() override
    {
        stopTimer();

        folder().createDirectory();
        folder().getChildFile ("unsaved.ini").replaceWithText (themeToIni (theme(), info));
    }

    void writeUnsavedIfPending()
    {
        if (isTimerRunning())
            timerCallback();
    }

    juce::String selectedId;
};
