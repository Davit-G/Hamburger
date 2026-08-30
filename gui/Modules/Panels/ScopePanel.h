#pragma once

#include "../Panel.h"
#include "../Scope.h"

class ScopePanel : public Panel
{
public:
    ScopePanel(AudioPluginAudioProcessor &p) : Panel(p, "SETTINGS"),
        scope(p.treeState, p.getScopeDataCollector(), p.getScopeContext()),
        lockOffImage(juce::ImageCache::getFromMemory(BinaryData::lockoff_png, BinaryData::lockoff_pngSize)),
        lockOnImage(juce::ImageCache::getFromMemory(BinaryData::lockon_png, BinaryData::lockon_pngSize))
    {
        addAndMakeVisible(scope);
        lockedButton = std::make_unique<LightButton>(p, lockOffImage, lockOnImage);
        addAndMakeVisible(*lockedButton);

        lockedButton->onClick = [this, &p]
        {
            p.getScopeContext().toggleLocked();
            repaint();
        };
        lockedButton->setAlpha(lockNormalAlpha);
        addMouseListener(this, true);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        scope.setBounds(bounds);

        constexpr int buttonSize = 16;
        constexpr int margin = 8;

        lockedButton->setBounds(
            bounds.getX() + margin,
            bounds.getBottom() - buttonSize - margin,
            buttonSize,
            buttonSize
        );
    }

    void doHoverAnim(bool isHovered)
    {
        if (lockedButton == nullptr)
            return;

        auto& animator = juce::Desktop::getInstance().getAnimator();

        animator.animateComponent(
            lockedButton.get(),
            lockedButton->getBounds(),
            isHovered ? lockHoverAlpha : lockNormalAlpha,
            isHovered ? static_cast<int>(lockHoverTime * 1.1f) : lockHoverTime,
            false,
            isHovered ? 0.5 : 0.0,
            isHovered ? 0.1 : 0.0
        );
    }

    void mouseEnter(const juce::MouseEvent&) override
    {
        doHoverAnim(true);
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        if (!isMouseOver(true))
            doHoverAnim(false);
    }

private:

    int lockHoverTime = 100;
    float lockHoverAlpha = 0.8f;
    float lockNormalAlpha = 0.3f;
    bool wasHovered = false;

    Scope<float> scope;
    std::unique_ptr<LightButton> lockedButton;
    juce::Image lockOffImage;
    juce::Image lockOnImage;
};