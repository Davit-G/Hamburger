#include "Scope.h"
#include "../../dsp/ModMatrix.h"

void Scope::recordModulators()
{
    static_assert(ModSources::count == std::tuple_size_v<decltype(modulatorHistory)>);

    for (int s = 0; s < ModSources::count; ++s)
        modulatorHistory[(size_t) s][modulatorHistoryNext] = matrix.latestValue(s);

    const auto connection = scopeContext.getModulatorConnection();

    if (connection >= 0)
    {
        if (connection != recordedConnection)
            connectionHistory.fill(matrix.shapedValue(connection));

        connectionHistory[modulatorHistoryNext] = matrix.shapedValue(connection);
    }

    recordedConnection = connection;

    modulatorHistoryNext = (modulatorHistoryNext + 1) % modulatorHistory[0].size();
}

// the source's output scrolling left, newest on the right, 0 at the bottom and 1 at the top
void Scope::drawModulatorView(juce::Graphics &g, juce::Rectangle<int> area)
{
    const auto scopeRect = area.toFloat();
    const auto source = juce::jlimit(0, ModSources::count - 1, scopeContext.getModulatorSource());
    const auto connection = scopeContext.getModulatorConnection();
    const auto& history = connection >= 0 ? connectionHistory : modulatorHistory[(size_t) source];

    drawTiledContextLabel(g, area, "MODULATION");

    const auto plot = scopeRect.withTrimmedTop(headerHeight(scopeRect)).reduced(4.0f, 6.0f);

    g.setColour(theme().scopeGrid);
    g.drawHorizontalLine(juce::roundToInt(plot.getCentreY()), plot.getX(), plot.getRight());

    juce::Path line;

    for (size_t i = 0; i < history.size(); ++i)
    {
        const auto value = history[(modulatorHistoryNext + i) % history.size()];
        const auto x = plot.getX() + plot.getWidth() * (float) i / (float) (history.size() - 1);
        const auto y = plot.getBottom() - plot.getHeight() * juce::jlimit(0.0f, 1.0f, value);

        if (i == 0)
            line.startNewSubPath(x, y);
        else
            line.lineTo(x, y);
    }

    g.setColour(theme().modSources[(size_t) source]);
    g.strokePath(line, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    drawParamHeader(g, scopeRect, { connection >= 0 ? matrix.connectionName(connection) : matrix.sourceName(source) });
}
