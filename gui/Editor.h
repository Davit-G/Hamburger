#pragma once

#include "../PluginProcessor.h"
#include "juce_gui_extra/juce_gui_extra.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "juce_core/juce_core.h"
#include "juce_audio_processors/juce_audio_processors.h"

#include "BinaryData.h"

#include "Info.h"
#include "LeftColumn.h"
#include "SaturationColumn.h"
#include "UtilColumn.h"

#include "PresetPanel.h"
#include "PluginHeader.h"
#include "UpdateChecker.h"

#include "LookAndFeel/HamburgerLAF.h"
#include "LookAndFeel/ThemeManager.h"

class EditorV2 : public juce::Component, public juce::ChangeListener
{
public:
    EditorV2(AudioPluginAudioProcessor &p) : audioProcessorRef(p),
                                             leftColumn(p),
                                             saturationColumn(p),
                                             utilColumn(p),
                                             infoPanel(p)
                                             ,presetPanel(p.getPresetManager()),
                                             header(p)
    {   
        // the first editor opened picks up the theme from last time, any others share it
        if (themes->getSelectedId().isEmpty() && ! themes->select(p.getAppProperties().appProperties.getUserSettings()->getValue("theme", ThemeManager::defaultId)))
            themes->select(ThemeManager::defaultId);

        themes->addChangeListener(this);

        setLookAndFeel(&hamburgerLAF);
        infoPanel.setLookAndFeel(&hamburgerLAF);
        leftColumn.setLookAndFeel(&hamburgerLAF);
        saturationColumn.setLookAndFeel(&hamburgerLAF);
        utilColumn.setLookAndFeel(&hamburgerLAF);
        presetPanel.setLookAndFeel(&hamburgerLAF);

        addAndMakeVisible(leftColumn);
        addAndMakeVisible(saturationColumn);
        addAndMakeVisible(utilColumn);
        addAndMakeVisible(infoPanel);
        addAndMakeVisible(header);
        header.onTabSelected = [this] (PluginHeader::Tab tab) { this->saturationColumn.setView (tab); };

        saturationColumn.setModuleColours ([this] (ModuleId id) {
            for (auto* module : { leftColumn.moduleFor (id), saturationColumn.moduleFor (id), utilColumn.moduleFor (id) })
                if (module != nullptr)
                    return (theme().*module->getAccent()).main;

            return theme().plain.main;
        });
        addAndMakeVisible(presetPanel);

        if (audioProcessorRef.getAppProperties().getTooltipType() == AppProperties::TooltipType::window) {
            createTooltipWindow();
        }

        setOpaque(true);

        // for 1 - 4 quick selection
        setWantsKeyboardFocus(true);

        infoPanel.setVisible(false);

        setPaintingIsUnclipped(true);

        p.getAppProperties().addChangeListener(this);

        updater = std::make_unique<UpdateChecker>(JucePlugin_VersionString); // change here to something random to test update mechanism

        updater->shouldCheckForUpdates = [this]() 
        {
            auto* props = audioProcessorRef.getAppProperties().appProperties.getUserSettings();
            if (props == nullptr) return true;

            if (!props->getBoolValue("check_for_updates", true))
                return false;

            juce::int64 lastCancelTime = props->getDoubleValue("last_update_cancel_time", 0.0);
            juce::int64 currentTime = juce::Time::getCurrentTime().toMilliseconds();
            juce::int64 twentyFourHoursInMs = 24LL * 60LL * 60LL * 1000LL;

            if (currentTime - lastCancelTime < twentyFourHoursInMs)
            {
                return false;
            }

            return true;
        };

        updater->onCancelUpdates = [this]() 
        {
            auto* props = audioProcessorRef.getAppProperties().appProperties.getUserSettings();
            if (props != nullptr)
            {
                props->setValue("last_update_cancel_time", 
                                (double)juce::Time::getCurrentTime().toMilliseconds());
                props->saveIfNeeded();
            }
        };

        updater->onDisableUpdates = [this]() 
        {
            auto* props = audioProcessorRef.getAppProperties().appProperties.getUserSettings();
            if (props != nullptr)
            {
                props->setValue("check_for_updates", false);
                props->saveIfNeeded();
            }
        };

        updater->checkForUpdates();

    }

    void createTooltipWindow() {
        tooltipWindow = std::make_unique<juce::TooltipWindow>(this, 600);
        tooltipWindow->setLookAndFeel(&hamburgerLAF);
        addAndMakeVisible(tooltipWindow.get());
    }

    ~EditorV2()
    {
        setLookAndFeel(nullptr);
        infoPanel.setLookAndFeel(nullptr);
        leftColumn.setLookAndFeel(nullptr);
        saturationColumn.setLookAndFeel(nullptr);
        utilColumn.setLookAndFeel(nullptr);
        presetPanel.setLookAndFeel(nullptr);
        if (tooltipWindow != nullptr) {
            tooltipWindow->setLookAndFeel(nullptr);
        }

        audioProcessorRef.getAppProperties().removeChangeListener(this);
        themes->removeChangeListener(this);
    }

    void changeListenerCallback (juce::ChangeBroadcaster* source) override {
        // every component reads its colours from theme() again, and repaints
        if (source == &themes.get())
            sendLookAndFeelChange();

        if (source == &audioProcessorRef.getAppProperties()) {
            bool displayTooltips = audioProcessorRef.getAppProperties().getTooltipType() == AppProperties::TooltipType::window;

            if (displayTooltips && tooltipWindow == nullptr) {
                createTooltipWindow();
            } else {
                tooltipWindow = nullptr;
            }
        }
    }

    void lookAndFeelChanged() override
    {
        hamburgerLAF.applyTheme();

        // drawing it is the slow part, so only when the themed svg comes out different
        auto svg = themes->backgroundSvg();
        applyThemeToSvg(*svg);

        if (const auto themed = svg->toString(); themed != backgroundSvg)
        {
            backgroundSvg = themed;
            background = {};
        }
    }

    // the svg is drawn once into an image, and again only when the theme or the scale changes
    void paint(juce::Graphics &g) override
    {
        const auto scale = g.getInternalContext().getPhysicalPixelScaleFactor();

        if (! background.isValid() || ! juce::approximatelyEqual(scale, backgroundScale))
        {
            backgroundScale = scale;
            background = makeBackground(backgroundSvg, juce::roundToInt((float) getWidth() * scale), juce::roundToInt((float) getHeight() * scale));
        }

        g.drawImage(background, getLocalBounds().toFloat());
    }

    // pressing 1-4 will switch positions on the main editor if we're in a multiband view or similar
    bool keyPressed(const juce::KeyPress &key) override
    {
        const auto character = key.getTextCharacter();

        if (key.getModifiers().isAnyModifierKeyDown() || character < '1' || character > '4')
            return false;

        saturationColumn.selectPosition((int) (character - '1'));
        return true;
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto totalWidth = bounds.getWidth() / 4;

        if (tooltipWindow != nullptr) {
            tooltipWindow->setBounds(bounds);
        }

        infoPanel.setBounds(bounds);

        header.setBounds(bounds.removeFromTop(PluginHeader::totalHeight));
        
        presetPanel.setBounds(getLocalBounds().withTrimmedRight(PluginHeader::rightReserved));

        auto left = bounds.removeFromLeft(totalWidth);
        auto right = bounds.removeFromRight(totalWidth);

        leftColumn.setBounds(left);
        saturationColumn.setBounds(bounds);
        utilColumn.setBounds(right);
    }

    void handleCommandMessage(int command) override
    {
        bool show = command == 1;

        infoPanel.setVisible(!show);
        
        leftColumn.setVisible(show);
        saturationColumn.setVisible(show);
        utilColumn.setVisible(show);
        header.setVisible(show);
        presetPanel.setVisible(show);
    }

private:
    static juce::Image makeBackground(const juce::String& svg, int width, int height)
    {
        auto drawable = juce::Drawable::createFromSVG(*juce::parseXML(svg));

        // shapes can run well past the svg's canvas, so it's the canvas that's fitted rather than the drawing's bounds
        const auto canvas = dynamic_cast<juce::DrawableComposite&>(*drawable).getContentArea();

        juce::Image image(juce::Image::RGB, width, height, true);
        juce::Graphics g(image);

        drawable->draw(g, 1.0f, juce::RectanglePlacement(juce::RectanglePlacement::fillDestination).getTransformToFit(canvas, image.getBounds().toFloat()));
        return image;
    }

    AudioPluginAudioProcessor& audioProcessorRef;
    juce::SharedResourcePointer<ThemeManager> themes;

    LeftColumn leftColumn;
    SaturationColumn saturationColumn;
    UtilColumn utilColumn;

    HamburgerLAF hamburgerLAF;

    std::unique_ptr<juce::TooltipWindow> tooltipWindow;

    PresetPanel presetPanel;
    PluginHeader header;
    std::unique_ptr<UpdateChecker> updater;

    Info infoPanel;

    juce::Image background;
    float backgroundScale = 0.0f;
    juce::String backgroundSvg;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditorV2)
};