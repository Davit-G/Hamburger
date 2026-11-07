#include "Scope.h"
#include "../../dsp/Distortions/waveshape/Waveshape.h"

// the in out trails, with the waveshape's curve behind them when that's the type being watched
void Scope::drawDistortionView(juce::Graphics &g, juce::Rectangle<int> area)
{
    const auto scopeRect = area.toFloat();

    if (inOutFB.isValid())
        g.drawImage(inOutFB, scopeRect);

    const auto withCurve = scopeContext.getType() == ScopeContextType::WAVESHAPE || focusIsWaveshape();

    if (withCurve)
        drawWaveshapeCurve(g, scopeRect);

    drawTiledContextLabel(g, area, withCurve ? "WAVESHAPE" : "DISTORTION");
    drawParamHeader(g, scopeRect, getDistortionHeaderLabels());
    drawDistortionAmount(g, scopeRect);
}

void Scope::drawInOut(juce::Graphics &g, juce::Rectangle<float> scopeRect)
{
    if (sampleDataPreDistortion.size() > 1 && sampleDataPostDistortion.size() > 1)
    {
        // centred under the header, so the top of the curve cannot run up behind it
        const auto plot = scopeRect.withTrimmedTop(headerHeight(scopeRect));

        const auto centerX = plot.getCentreX();
        const auto centerY = plot.getCentreY();
        const auto halfW = plot.getHeight() * float(0.95);
        const auto halfH = plot.getHeight() * float(0.46);

        g.setColour(theme().scopeLeft);
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

// the waveshape type's curve for the slot being watched. the table is only rebuilt when the blend or smoothing moves
struct WaveshapeCurveCache
{
    waveshapes::CurveTable table;
    waveshapes::Mix mix;
    double x = -1.0, y = -1.0, width = -1.0;

    void update(double newX, double newY, double newWidth)
    {
        if (newX == x && newY == y && newWidth == width)
            return;

        x = newX;
        y = newY;
        width = newWidth;
        mix = waveshapes::mixAt(waveshapes::mapPoints, (int) std::size(waveshapes::mapPoints), x, y);
        table.build(mix, width);
    }
};

bool Scope::focusIsWaveshape() const
{
    const auto focus = scopeContext.getFocus();

    // only the distortion slots can run it
    if (focus.module != ModuleId::main && focus.module != ModuleId::preDistortion && focus.module != ModuleId::postDistortion)
        return false;

    auto *type = choiceParamForSlot(focus);
    return type != nullptr && type->getCurrentChoiceName() == "WAVESHAPE";
}

void Scope::drawWaveshapeCurve(juce::Graphics &g, juce::Rectangle<float> scopeRect)
{
    const auto slot = scopeContext.getFocus();

    // as the audio hears it, so the drive shows global drive's scaling
    auto value = [this, slot](const ParamIDs::ParameterInfo &info) {
        auto *param = dynamic_cast<MacroParam*>(apvts.getParameter(paramIdFor(slot, info).getParamID()));
        return (double) (param != nullptr ? param->getScaled() : info.defaultValue);
    };

    auto flag = [this](const juce::ParameterID &id) {
        auto *param = apvts.getParameter(id.getParamID());
        return param != nullptr && param->getValue() >= 0.5f;
    };

    if (waveshapeCurve == nullptr)
        waveshapeCurve = std::make_shared<WaveshapeCurveCache>();

    auto &curve = *waveshapeCurve;
    curve.update(value(ParamIDs::waveshapeX), value(ParamIDs::waveshapeY),
                 waveshapes::smoothToWidth(value(ParamIDs::waveshapeSmooth) * 0.01));

    const auto enabled = flag(slot.enabled());
    const auto inDb = value(ParamIDs::slotInGain);
    const auto linked = enabled && flag(paramIdFor(slot, ParamIDs::gainLink));

    const auto inGain = juce::Decibels::decibelsToGain(inDb);
    const auto outGain = juce::Decibels::decibelsToGain(value(ParamIDs::slotOutGain) - (linked ? inDb : 0.0));
    const auto wet = value(ParamIDs::slotMix) * 0.01;

    const auto driveDb = value(ParamIDs::waveshapeDrive);
    const auto drive = juce::Decibels::decibelsToGain(driveDb);
    const auto offset = waveshapes::biasCurve(value(ParamIDs::waveshapeBias));
    const auto amount = value(ParamIDs::waveshapeAsym) * 0.01;

    const auto loudness = waveshapes::mixLoudness(curve.mix, driveDb, waveshapes::loudnessGains)
                        * waveshapes::mixRelevel(curve.mix, driveDb, waveshapes::loudnessGains);
    const auto rest = curve.table.read(std::tanh(waveshapes::skew(offset, amount)));

    auto transfer = [&](double in) {
        if (! enabled)
            return in * outGain;

        const auto driven = std::tanh(waveshapes::skew(in * inGain * drive + offset, amount));
        const auto shaped = (curve.table.read(driven) - rest) * loudness;

        return (shaped * wet + in * (1.0 - wet)) * outGain;
    };

    const auto plot = scopeRect.withTrimmedTop(headerHeight(scopeRect));
    const auto centreX = (float) plot.getCentreX();
    const auto centreY = (float) plot.getCentreY();
    const auto halfW = (float) plot.getHeight() * 0.95f;
    const auto halfH = (float) plot.getHeight() * 0.46f;

    auto pointAt = [&](float px) { return juce::Point<float>(px, centreY - (float) transfer((double) ((px - centreX) / halfW)) * halfH); };

    juce::Path path;
    path.startNewSubPath(pointAt((float) plot.getX()));

    for (auto px = (float) plot.getX() + 2.0f; px <= (float) plot.getRight(); px += 2.0f)
        path.lineTo(pointAt(px));

    g.saveState();
    g.reduceClipRegion(plot.toNearestInt());
    g.setColour(theme().scopeCurve);
    g.strokePath(path, juce::PathStrokeType(1.5f));
    g.restoreState();
}

// accumulates one frame of the trace into the framebuffer, called from the timer so trail
// density follows the frame rate rather than however often the os asks us to repaint
void Scope::renderInOutFrame(bool stampNewTrace)
{
    juce::Graphics imgG(inOutFB);

    // fading toward the background rather than clearing is what leaves the older frames behind
    imgG.setColour(theme().scopeBackground.withAlpha(inOutFade));
    imgG.fillRect(inOutFB.getBounds());

    if (!stampNewTrace)
        return;

    imgG.addTransform(juce::AffineTransform::scale(inOutRenderScale));
    drawInOut(imgG, juce::Rectangle<float>{0.0f, 0.0f,
                                                (float) getWidth(), (float) getHeight()});
}

// bottom right corner rather than the header, since that corner of the transfer plot stays empty
void Scope::drawDistortionAmount(juce::Graphics &g, juce::Rectangle<float> scopeRect)
{
    const auto label = getDistortionAmountLabel();

    if (label.isEmpty())
        return;

    g.setFont(getLookAndFeel().getPopupMenuFont());
    g.setFont(scope_constants::readoutFontHeight);

    const auto rowHeight = (float) headerHeight(scopeRect);
    const auto row = juce::Rectangle<float>(0.0f, (float) scopeRect.getHeight() - rowHeight,
                                            (float) scopeRect.getWidth(), rowHeight).reduced(4.0f, 0.0f);

    drawTabbedLabel(g, row, label, juce::Justification::centredRight, false);
}

juce::StringArray Scope::getDistortionHeaderLabels() const
{
    auto *type = choiceParamForSlot(SlotId{ModuleId::main, 0});

    if (type == nullptr)
        return {};

    return { type->getCurrentChoiceName() };
}

// every type keeps its own amount parameter on its own range, and matrix has no single one
juce::String Scope::getDistortionAmountLabel() const
{
    auto *type = choiceParamForSlot(SlotId{ModuleId::main, 0});

    if (type == nullptr)
        return {};

    switch (type->getIndex())
    {
        case 0: return formatPercent(paramValue(ParamIDs::saturationAmount) * 0.01f); // GRILL
        case 1: return formatPercent(paramValue(ParamIDs::tubeAmount) * 0.01f);       // TUBE
        case 2: return formatPercent(paramValue(ParamIDs::phaseAmount) * 0.01f);      // PHASE
        case 3: return formatPercent(paramValue(ParamIDs::rubidiumAmount) * 0.01f);   // RUBIDIUM
        case 4: return formatPercent(paramValue(ParamIDs::tapeDrive));                // TAPE
        case 5: return formatDecibels(paramValue(ParamIDs::alphaParam));               // SLEW
        case 6: return formatDecibels(paramValue(ParamIDs::waveshapeDrive));          // WAVESHAPE
        case 7: return formatDecibels(paramValue(ParamIDs::dllDrive));                // DLL, debug only
        default: return {};
    }
}
