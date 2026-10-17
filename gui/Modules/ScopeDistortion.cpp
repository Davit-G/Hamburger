#include "Scope.h"

// the in against out trace a timer tick has already rendered, with the type and its amount
void Scope::drawDistortionView(juce::Graphics &g, juce::Rectangle<int> area, SlotId slot)
{
    const auto scopeRect = area.toFloat();

    if (inOutFB.isValid())
        g.drawImage(inOutFB, scopeRect);

    drawTiledContextLabel(g, area, "DISTORTION");
    drawParamHeader(g, scopeRect, getDistortionHeaderLabels(slot));
    drawDistortionAmount(g, scopeRect, slot);

    // drawInOutAxes(g, scopeRect);
}

// accumulates one frame of the trace into the framebuffer, called from the timer so trail
// density follows the frame rate rather than however often the os asks us to repaint
void Scope::renderInOutFrame(bool stampNewTrace)
{
    juce::Graphics imgG(inOutFB);

    // fading toward the background rather than clearing is what leaves the older frames behind
    imgG.setColour(inOutBackground.withAlpha(inOutFade));
    imgG.fillRect(inOutFB.getBounds());

    if (!stampNewTrace)
        return;

    imgG.addTransform(juce::AffineTransform::scale(inOutRenderScale));
    drawInOut(imgG, juce::Rectangle<float>{0.0f, 0.0f,
                                                (float) getWidth(), (float) getHeight()});
}

void Scope::drawInOut(juce::Graphics &g, juce::Rectangle<float> scopeRect)
{
    if (sampleDataPreDistortion.size() > 1 && sampleDataPostDistortion.size() > 1)
    {
        // centred under the header, so the top of the curve cannot run up behind it
        const auto plot = scopeRect.withTrimmedTop(headerHeight(scopeRect));

        const auto centerX = plot.getCentreX();
        const auto centerY = plot.getCentreY();
        const auto halfW = plot.getHeight() * 0.95f;
        const auto halfH = plot.getHeight() * 0.46f;

        g.setColour(juce::Colours::yellow);
        const auto count = juce::jmin(sampleDataPreDistortion.size(), sampleDataPostDistortion.size());
        for (size_t i = 1; i < count; ++i)
        {
            const auto x0 = sampleDataPreDistortion[i - 1];
            const auto y0 = sampleDataPostDistortion[i - 1];
            const auto x1 = sampleDataPreDistortion[i];
            const auto y1 = sampleDataPostDistortion[i];

            const auto px0 = centerX + x0 * halfW;
            const auto py0 = centerY - y0 * halfH;
            const auto px1 = centerX + x1 * halfW;
            const auto py1 = centerY - y1 * halfH;

            g.drawLine(px0, py0, px1, py1);
        }
    }
}

// bottom right corner rather than the header, since that corner of the transfer plot stays empty
void Scope::drawDistortionAmount(juce::Graphics &g, juce::Rectangle<float> scopeRect, SlotId slot)
{
    const auto label = getDistortionAmountLabel(slot);

    if (label.isEmpty())
        return;

    g.setFont(getLookAndFeel().getPopupMenuFont());
    g.setFont(scope_constants::readoutFontHeight);

    const auto rowHeight = (float) headerHeight(scopeRect);
    const auto row = juce::Rectangle<float>(0.0f, (float) scopeRect.getHeight() - rowHeight,
                                            (float) scopeRect.getWidth(), rowHeight).reduced(4.0f, 0.0f);

    drawTabbedLabel(g, row, label, juce::Justification::centredRight, false);
}

juce::StringArray Scope::getDistortionHeaderLabels(SlotId slot) const
{
    auto *type = dynamic_cast<juce::AudioParameterChoice *>(apvts.getParameter(slot.type().getParamID()));

    if (type == nullptr)
        return {};

    return { type->getCurrentChoiceName() };
}

// every type keeps its own amount parameter on its own range, and matrix has no single one
juce::String Scope::getDistortionAmountLabel(SlotId slot) const
{
    auto *type = dynamic_cast<juce::AudioParameterChoice *>(apvts.getParameter(slot.type().getParamID()));

    if (type == nullptr)
        return {};

    switch (type->getIndex())
    {
        case 0: return formatPercent(getValue(ParamIDs::saturationAmount, slot) * 0.01f); // GRILL
        case 1: return formatPercent(getValue(ParamIDs::tubeAmount, slot) * 0.01f);       // TUBE
        case 2: return formatPercent(getValue(ParamIDs::phaseAmount, slot) * 0.01f);      // PHASE
        case 3: return formatPercent(getValue(ParamIDs::rubidiumAmount, slot) * 0.01f);   // RUBIDIUM
        case 4: return formatPercent(getValue(ParamIDs::tapeDrive, slot));                // TAPE
        case 5: return formatPercent(getValue(ParamIDs::alphaParam, slot));               // SLEW
        case 6: return formatDecibels(getValue(ParamIDs::waveshapeDrive, slot));          // WAVESHAPE
        case 8: return formatDecibels(getValue(ParamIDs::dllDrive, slot));                // DLL, debug only
        default: return {};
    }
}
