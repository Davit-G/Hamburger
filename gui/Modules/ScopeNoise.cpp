#include "Scope.h"

// a sine through the scope's own copy of the noise dsp, to show what it does to a wave
void Scope::drawNoiseView(juce::Graphics &g, juce::Rectangle<int> area, SlotId slot)
{
    const auto scopeRect = area.toFloat();

    drawTiledContextLabel(g, area, "NOISE");

    for (int i = 0; i < noiseDistBuf.getNumSamples(); i++) {
        noiseDistBuf.setSample(0, i, 0.7f * sin(6.28f * (i - 32.0f) * 100.f / 44100.f));
    }
    auto block = juce::dsp::AudioBlock<float>(noiseDistBuf);
    noiseDist.processBlock(block);

    const auto noiseRect = scopeRect.withTrimmedTop(headerHeight(scopeRect));

    // draw line halfway
    g.setColour(juce::Colours::darkgrey);
    g.drawLine(0.0f, noiseRect.getCentreY(), scopeRect.getWidth(), noiseRect.getCentreY());

    // draw wave
    g.setColour(juce::Colours::yellow);
    plotStraightLine(noiseDistBuf.getReadPointer(0) + 32, noiseDistBuf.getNumSamples() - 32, g, noiseRect, 0.5f, noiseRect.getHeight() / 2);

    drawParamHeader(g, scopeRect, getNoiseHeaderLabels(slot));
}

// only the sizzles and erosion have the full frequency / amount / q set
juce::StringArray Scope::getNoiseHeaderLabels(SlotId slot) const
{
    auto *type = dynamic_cast<juce::AudioParameterChoice *>(apvts.getParameter(slot.type().getParamID()));

    if (type == nullptr)
        return {};

    switch (type->getIndex())
    {
        case 1: // EROSION
            return { formatFrequency(getValue(ParamIDs::erosionFrequency, slot)),
                     formatPercent(getValue(ParamIDs::erosionAmount, slot) * 0.01f),
                     "Q " + juce::String(getValue(ParamIDs::erosionQ, slot), 2) };

        case 2: // BIT, no q - the bit depth is the interesting middle value instead
            return { formatFrequency(getValue(ParamIDs::downsampleFreq, slot)),
                     juce::String(juce::roundToInt(getValue(ParamIDs::bitReduction, slot))) + " bit"};

        case 3: // JEFF, the gate only has amount and mix
            return { formatPercent(getValue(ParamIDs::gateAmt, slot)),
                     "MIX " + formatPercent(getValue(ParamIDs::gateSmooth, slot)) };

        default: // SIZZLE and SIZZLE_OG
            return { formatFrequency(getValue(ParamIDs::sizzleFrequency, slot)),
                     formatPercent(getValue(ParamIDs::sizzleAmount, slot) * 0.01f),
                     "Q " + juce::String(getValue(ParamIDs::sizzleQ, slot), 2) };
    }
}
