#pragma once

#include "../Panel.h"
#include "../Scope.h"
#include "../LightButton.h"

class ScopePanel : public Panel
{
public:
    ScopePanel(AudioPluginAudioProcessor &p) : Panel(p, "SETTINGS"),
        context(p.getScopeContext()),
        scope(p.treeState, p.getScopeDataCollector(), p.getScopeContext(), p.getModMatrix())
    {
        addAndMakeVisible(scope);
        lockedButton = std::make_unique<LightButton>(lockGlyph(), &Theme::powerOn, &Theme::powerOff);
        addAndMakeVisible(*lockedButton);

        lockedButton->setTooltip("Locks the scope's view. Right click the scope to pick a view");

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

        constexpr int buttonSize = 12;
        constexpr int margin = 4;

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

    // right clicking anywhere on it locks it, or picks which view it shows
    void mouseDown(const juce::MouseEvent& e) override
    {
        if (! e.mods.isPopupMenu())
            return;

        juce::PopupMenu views;

        for (int type = 0; type < (int) std::size(scopeContextNames); ++type)
            views.addItem(scopeContextNames[type], true, context.getType() == type, [this, type] { context.setType((ScopeContextType) type, true); });

        juce::PopupMenu menu;
        menu.setLookAndFeel(&getLookAndFeel());
        menu.addItem("Lock", true, context.isLocked(), [this] { lockedButton->triggerClick(); });
        menu.addSubMenu("Change view", views);
        menu.showMenuAsync(juce::PopupMenu::Options().withMousePosition());
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

    ScopeContext& context;
    Scope scope;
    std::unique_ptr<LightButton> lockedButton;
};