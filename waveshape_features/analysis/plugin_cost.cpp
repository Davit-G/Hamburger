/*  What the waveshape module costs, group by group, at the slowest spot in each (the position blending the
    slowest pair of shapes). Build it optimised, the plugin is -O3. Per sample figures are per channel; a
    sample at 44.1 kHz leaves 22676 ns, a 512 sample block 11.6 ms.

      exact:    smooth at 0, the plugin plays the morph -- up to two shape calls per sample
      table:    smooth above 0, one table read per sample (two while a knob change fades)
      rebuild:  a table build, which happens once for every block a curve knob (type, shapes, smooth) moved */

#define main features_main
#include "../features.cpp"
#undef main

#ifndef HAVE_ORDER_HEADER
#error "generate WaveshapeOrder.h first: waveshape_features --header > dsp/Distortions/waveshape/WaveshapeOrder.h"
#endif

#include <random>

int main() {
    const int N = 20000;
    std::vector<double> input(N);
    std::mt19937 rng(1);
    std::uniform_real_distribution<double> uniform(-1.0, 1.0);
    for (double& v : input) v = std::tanh(2.0 * uniform(rng));

    auto fastest = [](auto&& run) {
        double best = 1e300;
        for (int attempt = 0; attempt < 3; attempt++) {
            auto start = std::chrono::steady_clock::now();
            run();
            best = std::min(best, std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - start).count());
        }
        return best;
    };

    static CurveTable table;
    volatile double sink = 0.0; // keeps the timed loops from being optimised away

    printf("%-28s %18s %14s %22s\n", "group", "exact, worst", "table", "rebuild, worst");
    for (int g = 0; g < GroupCount; g++) {
        const auto& order = groupOrders[g];
        double worstExact = 0.0, worstRebuild = 0.0;
        const char* exactName = "", *rebuildName = "";

        for (int i = 0; i + 1 < order.count; i++) {
            const double position = i + 0.5;
            const double exact = fastest([&] { for (int n = 0; n < N; n++) sink = sink + morph(order, position, input[n]); }) / N;
            if (exact > worstExact) { worstExact = exact; exactName = shapes[order.entries[i].index].name; }

            const double rebuild = fastest([&] { table.build(order, position, maxSmoothWidth); });
            if (rebuild > worstRebuild) { worstRebuild = rebuild; rebuildName = shapes[order.entries[i].index].name; }
        }

        const double read = fastest([&] { for (int n = 0; n < N; n++) sink = sink + table.read(input[n]); }) / N;
        printf("%-28s %9.1f ns (%s) %8.1f ns %12.2f ms (%s)\n", groupNames[g], worstExact, exactName, read, worstRebuild / 1e6, rebuildName);
    }
}
