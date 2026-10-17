#include "Scope.h"
#include "../../dsp/WaveShapers.h"

void Scope::drawClipperView(juce::Graphics &g, juce::Rectangle<int> area, SlotId slot)
{
    const auto scopeRect = area.toFloat();

    drawTiledContextLabel(g, area, "CLIPPER");

    const auto  w = (float) scopeRect.getWidth();
    const auto h = (float) scopeRect.getHeight();

    const auto knee = postClipKneeParam != nullptr ? postClipKneeParam->get() * 0.5f : 0.0f;

    constexpr float threshold = 1.0f;
    constexpr float maxIn = 2.5f;
    constexpr float maxOut = 1.25f;

    // mapping between pixel space and cartesian plane type coords, with the header strip kept clear
    const auto plot = scopeRect.withTrimmedTop(headerHeight(scopeRect));

    auto toX = [w](float in) { return juce::jmap(in, 0.0f, maxIn, 0.0f, w); };
    auto toY = [&plot](float out) { return juce::jmap(out, 0.0f, maxOut, (float) plot.getBottom() - 1.0f, (float) plot.getY() + 1.0f); };

    // shaded knee region
    if (knee > 0.0f)
    {
        const auto kneeStart = toX(threshold - knee * 0.5f);
        const auto kneeEnd = toX(threshold + knee * 0.5f);

        // translucent rather than a solid fill, so the watermark underneath still reads through it -
        // alpha 22 over black lands on the same 22,22,22 the solid version used to paint
        g.setColour(juce::Colour::fromRGBA(255, 255, 255, 22));
        g.fillRect(juce::Rectangle<float>(kneeStart, 0.0f, kneeEnd - kneeStart, h));
    }

    // line at 1.0
    g.setColour(juce::Colours::darkgrey);
    g.drawLine(0.0f, toY(0.0f), w, toY(0.0f));

    // f(x) = x
    g.setColour(juce::Colour::fromRGB(64, 64, 64));
    g.drawLine(toX(0.0f), toY(0.0f), toX(maxOut), toY(maxOut));

    // where the flat top lands
    g.drawLine(0.0f, toY(threshold), w, toY(threshold));

    constexpr int numPoints = 64;

    juce::Path curve;
    for (int i = 0; i <= numPoints; ++i)
    {
        const float in = juce::jmap((float) i, 0.0f, (float) numPoints, 0.0f, maxIn);
        const float x = toX(in);
        const float y = toY(softClipperFunc(in, threshold, knee));

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

    g.setColour(juce::Colours::white.withAlpha(0.05f));
    g.fillPath(closeToBaseline(curve, toX(maxIn)));

    g.setColour(juce::Colours::darkgrey);
    g.strokePath(curve, juce::PathStrokeType(2.0f));
    
    const auto level = juce::jlimit(0.0f, maxIn, dataCollector.levelMeter.getNext());

    auto levelColour = juce::Colours::yellow;
    if (level > threshold + knee * 0.5f)
        levelColour = juce::Colours::red;
    else if (isSoftClipperKnee(level, threshold, knee))
        levelColour = juce::Colours::orange;

    g.setColour(levelColour);
    
    if (level > 0.0f)
    {
        juce::Path active;
        for (int i = 0; i <= numPoints; ++i)
        {
            const auto in = juce::jmap((float) i, 0.0f, (float) numPoints, 0.0f, level);
            const auto x = toX(in);
            const auto y = toY(softClipperFunc(in, threshold, knee));

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
    const auto outAtLevel = softClipperFunc(level, threshold, knee);

    g.fillEllipse(toX(level) - dotRadius, toY(outAtLevel) - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);



    drawParamHeader(g, scopeRect, { formatDecibels(getValue(ParamIDs::postClipGain, slot)),
                                    juce::String(getValue(ParamIDs::postClipKnee, slot), 1) + " dB" });
}
