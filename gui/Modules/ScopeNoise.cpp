#include "Scope.h"

// a sine put through a copy of the noise module, to show what it does
void Scope::drawNoiseView(juce::Graphics &g, juce::Rectangle<int> area)
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
    g.setColour(theme().scopeGrid);
    g.drawLine(0.0f, noiseRect.getCentreY(), scopeRect.getWidth(), noiseRect.getCentreY());

    // draw wave
    g.setColour(theme().scopeLeft);
    plotStraightLine(noiseDistBuf.getReadPointer(0) + 32, noiseDistBuf.getNumSamples() - 32, g, noiseRect, 0.5f, noiseRect.getHeight() / 2);

    drawParamHeader(g, scopeRect, getNoiseHeaderLabels());
}

// only the sizzles and erosion have the full frequency / amount / q set
juce::StringArray Scope::getNoiseHeaderLabels() const
{
    auto *type = choiceParamForSlot(SlotId{ModuleId::module1, 0});

    if (type == nullptr)
        return {};

    switch (type->getIndex())
    {
        case 1: // EROSION
            return { formatFrequency(paramValue(ParamIDs::erosionFrequency)),
                     formatPercent(paramValue(ParamIDs::erosionAmount) * 0.01f),
                     "Q " + juce::String(paramValue(ParamIDs::erosionQ), 2) };

        case 2: // BIT, no q - the bit depth is the interesting middle value instead
            return { formatFrequency(paramValue(ParamIDs::downsampleFreq)),
                     juce::String(juce::roundToInt(paramValue(ParamIDs::bitReduction))) + " bit" };

        case 3: // JEFF, the gate only has amount and smoothing
            return { formatPercent(paramValue(ParamIDs::gateAmt)),
                     "SMOOTH " + formatPercent(paramValue(ParamIDs::gateSmooth) * 0.01f) };

        default: // SIZZLE and FIZZ
            return { formatFrequency(paramValue(ParamIDs::sizzleFrequency)),
                     formatPercent(paramValue(ParamIDs::sizzleAmount) * 0.01f),
                     "Q " + juce::String(paramValue(ParamIDs::sizzleQ), 2) };
    }
}
