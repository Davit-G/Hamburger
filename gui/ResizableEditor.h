#pragma once

#include "../PluginProcessor.h"
#include "juce_gui_extra/juce_gui_extra.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "juce_core/juce_core.h"

#include "Editor.h"

#include "LookAndFeel/HamburgerLAF.h"

// use our own custom constrainer so we can force snap the sizes to integers (to make it less annoying)
class ScaleConstrainer : public juce::ComponentBoundsConstrainer
{
public:
    ScaleConstrainer(float baseW, float baseH) : baseWidth(baseW), baseHeight(baseH) {}

    static constexpr float scaleStep = 0.01f;

    void checkBounds(juce::Rectangle<int> &bounds,
                     const juce::Rectangle<int> &previousBounds,
                     const juce::Rectangle<int> &limits,
                     bool isStretchingTop, bool isStretchingLeft,
                     bool isStretchingBottom, bool isStretchingRight) override
    {
        juce::ComponentBoundsConstrainer::checkBounds(bounds, previousBounds, limits,
                                                      isStretchingTop, isStretchingLeft,
                                                      isStretchingBottom, isStretchingRight);

        auto scale = juce::jlimit((float)getMinimumWidth() / baseWidth,
                                  (float)getMaximumWidth() / baseWidth,
                                  std::round(((float)bounds.getWidth() / baseWidth) / scaleStep) * scaleStep);

        bounds.setSize(juce::roundToInt(baseWidth * scale), juce::roundToInt(baseHeight * scale));

        if (isStretchingLeft)
            bounds.setX(bounds.getRight() - bounds.getWidth());

        if (isStretchingTop)
            bounds.setY( bounds.getBottom() - bounds.getHeight());
    }

private:
    float baseWidth;
    float baseHeight;
};

class ResizableEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    ResizableEditor (AudioPluginAudioProcessor &p) : AudioProcessorEditor (&p), editor(p)
    {
        addAndMakeVisible (editor);

        setResizable(true, true);

        scaleConstrainer.setSizeLimits(juce::roundToInt(baseWidth * minScale), juce::roundToInt(baseHeight * minScale),
                                       juce::roundToInt(baseWidth * maxScale), juce::roundToInt(baseHeight * maxScale));
        scaleConstrainer.setFixedAspectRatio(baseWidth / baseHeight);

        setConstrainer(&scaleConstrainer);

        // load scaling preference from user settings
        propertiesFile = p.getAppProperties().appProperties.getUserSettings();
        if (propertiesFile != nullptr) {
            scaleFactor = propertiesFile->getDoubleValue(scalingKey, 1.0);
            setSize(baseWidth * scaleFactor, baseHeight * scaleFactor);
        } else {
            setSize(baseWidth, baseHeight);
        }

        readoutEnabled = true;
    }

    void timerCallback() override // on drag
    {
        stopTimer();

        if (showScaleReadout)
        {
            showScaleReadout = false;
            repaint();
        }

        if (propertiesFile != nullptr)
        {
            propertiesFile->setValue(scalingKey, scaleFactor);
            propertiesFile->saveIfNeeded();
        } else {
            DBG("OI properties not saving");
        }
    }

    void resized() override
    {
        scaleFactor = (float)getWidth() / baseWidth;

        editor.setTransform (juce::AffineTransform::scale (scaleFactor));
        editor.setBounds (0, 0, baseWidth, baseHeight);

        if (readoutEnabled)
            showScaleReadout = true;

        startTimer(500);
    }

    void paintOverChildren (juce::Graphics &g) override
    {
        if (!showScaleReadout)
            return;

        auto text = juce::String("Scale: " + juce::String(juce::roundToInt(scaleFactor * 100.0f))) + "%";

        auto font = juce::Font(juce::FontOptions(readoutFontHeight));

        if (auto *laf = dynamic_cast<HamburgerLAF *>(&editor.getLookAndFeel()))
            font = laf->getQuicksandFont().withHeight(readoutFontHeight);

        g.setFont(font);

        auto box = juce::Rectangle<float>(juce::GlyphArrangement::getStringWidth(font, text) + readoutPaddingX,
                                          readoutFontHeight + readoutPaddingY)
                       .withCentre(getLocalBounds().toFloat().getCentre());

        g.setColour(juce::Colours::black.withAlpha(0.85f));
        g.fillRoundedRectangle(box, 5.0f);

        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.drawRoundedRectangle(box.reduced(0.5f), 5.0f, 2.0f);

        g.setColour(juce::Colours::white);
        g.drawText(text, box, juce::Justification::centred);
    }

private:
    EditorV2 editor;

    const std::string scalingKey = "scaling_preference";

    juce::PropertiesFile *propertiesFile;

    float baseWidth = 800.f;
    float baseHeight = 545.f;
    float scaleFactor = 1.0f;

    float minScale = 0.5f;
    float maxScale = 2.0f;

    ScaleConstrainer scaleConstrainer {baseWidth, baseHeight};

    bool showScaleReadout = false; // hides or shows the scale square in center of the screen
    bool readoutEnabled = false; // only show when we're done creating the component

    static constexpr float readoutFontHeight = 26.0f;
    static constexpr float readoutPaddingX = 32.0f;
    static constexpr float readoutPaddingY = 20.0f;
};