#include "Scope.h"

float Scope::headerHeight(juce::Rectangle<float> scopeRect) const
{
    return (float) juce::jmin(16.0f, (float) scopeRect.getHeight() * 0.2f);
}

// readouts across the top of a view, ends hugging the corners. three already fill the strip so they
// get one solid bar, fewer than that get a tab each and leave the gaps open
void Scope::drawParamHeader(juce::Graphics &g, juce::Rectangle<float> scopeRect, const juce::StringArray &labels)
{
    if (labels.isEmpty())
        return;

    const auto bar = juce::Rectangle<float>(0.0f, 0.0f, (float) scopeRect.getWidth(), (float) headerHeight(scopeRect));

    g.setFont(getLookAndFeel().getPopupMenuFont());
    g.setFont(scope_constants::readoutFontHeight);

    const auto solidBar = labels.size() > 2;

    if (solidBar)
    {
        g.setColour(juce::Colours::black);
        g.fillRect(bar);
    }

    const auto cells = bar.reduced(4.0f, 0.0f);
    const auto cellWidth = cells.getWidth() / (float) labels.size();

    for (int i = 0; i < labels.size(); ++i)
    {
        const auto isFirst = i == 0;
        const auto isLast = i == labels.size() - 1;
        const auto justification = isFirst ? juce::Justification::centredLeft
                                           : (isLast ? juce::Justification::centredRight
                                                     : juce::Justification::centred);

        const auto cell = cells.withX(cells.getX() + (float) i * cellWidth).withWidth(cellWidth);

        if (solidBar)
        {
            g.setColour(juce::Colours::grey);
            g.drawText(labels[i], cell, justification, false);
        }
        else
        {
            drawTabbedLabel(g, cell, labels[i], justification, true);
        }
    }
}

void Scope::drawTabbedLabel(juce::Graphics &g, juce::Rectangle<float> cell, const juce::String &text,
                                        juce::Justification justification, bool hangingFromTop)
{
    if (text.isEmpty())
        return;

    constexpr auto corner = 5.0f;

    auto tab = cell.withWidth(juce::jmin(cell.getWidth(),
                                         juce::GlyphArrangement::getStringWidth(g.getCurrentFont(), text) + corner * 2.0f));

    const auto horizontal = justification.getOnlyHorizontalFlags();

    if (horizontal == juce::Justification::right)
        tab.setX(cell.getRight() - tab.getWidth());
    else if (horizontal == juce::Justification::horizontallyCentred)
        tab.setCentre(cell.getCentreX(), tab.getCentreY());

    // grown past the outer edge so only the inner corners round off
    const auto grown = hangingFromTop ? tab.withTop(tab.getY() - corner)
                                      : tab.withBottom(tab.getBottom() + corner);

    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle(grown.expanded(corner * 0.5f, 0.0f), corner);

    g.setColour(juce::Colours::grey);
    g.drawText(text, cell, justification, false);
}

// watermark naming the current context, tiled over the whole view with odd rows staggered and
// every row started off screen so the pattern runs past the edges
void Scope::drawTiledContextLabel(juce::Graphics &g, juce::Rectangle<int> area, const juce::String &text)
{
    const auto w = (float) area.getWidth();
    const auto h = (float) area.getHeight();

    if (w <= 0.0f || h <= 0.0f)
        return;

    const auto font = getLookAndFeel().getPopupMenuFont().withHeight(juce::jlimit(12.0f, 34.0f, h * 0.22f));

    g.setFont(font);
    g.setColour(juce::Colour::fromRGBA(255, 255, 255, scope_constants::contextLabelAlpha));

    const auto rowHeight = font.getHeight();
    const auto tileWidth = juce::GlyphArrangement::getStringWidth(font, text);

    if (tileWidth <= 0.0f || rowHeight <= 0.0f)
        return;

    // one long pre-repeated string per row, so the font lays out the joins - drawing each repeat
    // separately would round every start x and leave a seam
    juce::String row;

    for (int repeat = (int) std::ceil((w + tileWidth * 2.0f) / tileWidth); repeat > 0; --repeat)
        row << text;

    int rowIndex = 0;

    for (auto y = -rowHeight * 0.3f; y < h; y += rowHeight, ++rowIndex)
    {
        // both the bleed and the stagger pull left, or a staggered row opens a gap at the left edge
        const auto startX = -tileWidth * 0.35f - (rowIndex % 2 == 1 ? tileWidth * 0.5f : 0.0f);

        g.drawSingleLineText(row, juce::roundToInt(startX), juce::roundToInt(y + font.getAscent()));
    }
}
