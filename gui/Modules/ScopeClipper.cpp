#include "Scope.h"
#include "../../dsp/WaveShapers.h"

void Scope::drawClipperView(juce::Graphics &g, juce::Rectangle<int> area)
{
    const auto scopeRect = area.toFloat();

    drawTiledContextLabel(g, area, "CLIPPER");
    drawClipper(g, scopeRect);

    // the limiter shows its release where the clips show their knee, and the second fold its highpass after it
    const auto* type = choiceParamForSlot(SlotId{ModuleId::postClip, 0});
    const auto index = type != nullptr ? type->getIndex() : 0;

    juce::StringArray labels { formatDecibels(paramValue(ParamIDs::postClipGain)),
                               index == 1 ? juce::String(paramValue(ParamIDs::postClipTime), 0) + " ms"
                                          : juce::String(paramValue(ParamIDs::postClipKnee), 1) + " dB" };

    if (index == 3)
        labels.add(formatFrequency(paramValue(ParamIDs::postClipFoldFreq)));

    drawParamHeader(g, scopeRect, labels);
}

void Scope::drawClipper(juce::Graphics &g, juce::Rectangle<float> scopeRect)
{
    const auto w = (float) scopeRect.getWidth();
    const auto h = (float) scopeRect.getHeight();

    // the limiter never lets anything past 0db, which is a knee of nothing
    const auto* clipType = choiceParamForSlot(SlotId{ModuleId::postClip, 0});
    const auto limiting = clipType != nullptr && clipType->getIndex() == 1;
    const auto folding = clipType != nullptr && clipType->getIndex() == 2;
    const auto knee = postClipKneeParam != nullptr && ! limiting ? postClipKneeParam->get() : 0.0f;

    /*  The first fold takes everything the clip took off back off again, mirroring the peaks down under the ceiling. The
        second only folds the top end of it, so for anything slow enough to draw here it's the plain clip. The fold eases
        into nothing through the knee and stays there, like the dsp */
    const auto makeup = folding ? 1.0f / foldPeak(knee) : 1.0f;
    auto shape = [folding, knee, makeup](float in) { return folding ? foldShape(in, knee, makeup) : softClipperFunc(in, knee); };

    // the fold goes on through nothing to the other ceiling, so it gets the room below as well and the input to get there
    constexpr float threshold = 1.0f;
    const auto maxIn = folding ? 3.5f : 2.5f;
    constexpr float maxOut = 1.25f;
    const auto minOut = folding ? -maxOut : 0.0f;

    // mapping between pixel space and cartesian plane type coords, with the header strip kept clear
    const auto plot = scopeRect.withTrimmedTop(headerHeight(scopeRect));

    auto toX = [w, maxIn](float in) { return juce::jmap(in, 0.0f, maxIn, 0.0f, w); };
    auto toY = [&plot, minOut, maxOut](float out) { return juce::jmap(out, minOut, maxOut, (float) plot.getBottom() - 1.0f, (float) plot.getY() + 1.0f); };

    // shaded knee region
    if (knee > 0.0f)
    {
        // the knee is in dB, either side of the threshold
        const auto kneeStart = toX(threshold * juce::Decibels::decibelsToGain(-knee * 0.5f));
        const auto kneeEnd = toX(threshold * juce::Decibels::decibelsToGain(knee * 0.5f));

        // translucent rather than a solid fill, so the watermark underneath still reads through it -
        // alpha 22 over black lands on the same 22,22,22 the solid version used to paint
        g.setColour(theme().scopeClipKneeRegion);
        g.fillRect(juce::Rectangle<float>(kneeStart, 0.0f, kneeEnd - kneeStart, h));
    }

    // line at 1.0
    g.setColour(theme().scopeGrid);
    g.drawLine(0.0f, toY(0.0f), w, toY(0.0f));

    // f(x) = x
    g.setColour(theme().scopeGrid);
    g.drawLine(toX(0.0f), toY(0.0f), toX(maxOut), toY(maxOut));

    // where the flat top lands
    g.drawLine(0.0f, toY(threshold), w, toY(threshold));

    constexpr int numPoints = 64;

    juce::Path curve;
    for (int i = 0; i <= numPoints; ++i)
    {
        const float in = juce::jmap((float) i, 0.0f, (float) numPoints, 0.0f, maxIn);
        const float x = toX(in);
        const float y = toY(shape(in));

        if (i == 0)
            curve.startNewSubPath(x, y);
        else
            curve.lineTo(x, y);
    }

    // translucent, so the knee strip and the watermark still read through the fill
    auto closeToBaseline = [&](juce::Path path, float rightX) {
        path.lineTo(rightX, toY(0.0f));
        path.lineTo(toX(0.0f), toY(0.0f));
        path.closeSubPath();
        return path;
    };

    g.setColour(theme().scopeClipFill);
    g.fillPath(closeToBaseline(curve, toX(maxIn)));

    g.setColour(theme().scopeCurve);
    g.strokePath(curve, juce::PathStrokeType(2.0f));
    
    const auto level = juce::jlimit(0.0f, maxIn, dataCollector.levelMeter.getNext());

    auto levelColour = theme().scopeClipBelow;
    if (level > threshold * juce::Decibels::decibelsToGain(knee * 0.5f))
        levelColour = theme().clipHard;
    else if (isSoftClipperKnee(level, knee))
        levelColour = theme().clipKnee;

    g.setColour(levelColour);
    
    if (level > 0.0f)
    {
        juce::Path active;
        for (int i = 0; i <= numPoints; ++i)
        {
            const auto in = juce::jmap((float) i, 0.0f, (float) numPoints, 0.0f, level);
            const auto x = toX(in);
            const auto y = toY(shape(in));

            if (i == 0)
                active.startNewSubPath(x, y);
            else
                active.lineTo(x, y);
        }

        g.setColour(levelColour.withAlpha(0.18f));
        g.fillPath(closeToBaseline(active, toX(level)));

        g.setColour(levelColour);
        g.strokePath(active, juce::PathStrokeType(2.5f));
    }

    constexpr auto dotRadius = 3.0f;
    const auto outAtLevel = shape(level);

    g.fillEllipse(toX(level) - dotRadius, toY(outAtLevel) - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
}
