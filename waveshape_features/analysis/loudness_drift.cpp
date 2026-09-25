/*  How far each shape's loudness drifts off target between the loudness table's drive steps, for a few step
    sizes. Wrapping and bit shapes jump in level as the sine's peak crosses a wrap, which interpolating between
    steps can't follow, so finer steps shrink the drift -- and grow WaveshapeOrder.h. waveshape_features fails
    past 1 dB at the step actually used (driveStepDb in Waveshapes.h); this is for picking that step.

      loudness_drift [step dB ...]     default 1 0.5 0.25, each a multiple of 0.25 */

#define main features_main
#include "../features.cpp"
#undef main

int main(int argc, char** argv) {
    std::vector<double> steps;
    for (int i = 1; i < argc; i++) steps.push_back(std::atof(argv[i]));
    if (steps.empty()) steps = { 1.0, 0.5, 0.25 };

    // measured once, on a grid every step and its midpoints land on
    const double fine = 0.125;
    const int points = int(maxDriveDb / fine) + 1;
    std::vector<std::vector<double>> rms(points, std::vector<double>(shapes.size())); // [point][shape]
    for (int p = 0; p < points; p++)
        for (size_t s = 0; s < shapes.size(); s++)
            rms[p][s] = loudnessOf(shapes[s], p * fine);

    for (double step : steps) {
        const long every = std::lround(step / fine);
        if (every < 2 || every % 2 != 0 || std::fabs(every * fine - step) > 1e-9) {
            fprintf(stderr, "%g dB: steps have to be multiples of 0.25 dB\n", step);
            return 1;
        }
        const int count = int(maxDriveDb / step) + 1;

        // the same rule the real table uses, at this step size
        std::vector<double> targets(count);
        std::vector<std::vector<double>> gains(count); // [step][shape]
        for (int k = 0; k < count; k++)
            targets[k] = loudnessStep(rms[k * every], gains[k]);

        struct Row { const char* name; double drift, driveDb; };
        std::vector<Row> rows;
        for (size_t s = 0; s < shapes.size(); s++) {
            Row row { shapes[s].name, 0.0, 0.0 };
            for (int k = 0; k + 1 < count; k++) {
                if (gains[k][s] >= maxLoudnessGain || gains[k + 1][s] >= maxLoudnessGain) continue; // held at max, not aiming for the target
                double level = rms[k * every + every / 2][s] * 0.5 * (gains[k][s] + gains[k + 1][s]);
                double drift = std::fabs(20.0 * std::log10(level / std::sqrt(targets[k] * targets[k + 1])));
                if (drift > row.drift) row = { shapes[s].name, drift, (k + 0.5) * step };
            }
            rows.push_back(row);
        }

        std::sort(rows.begin(), rows.end(), [](auto& a, auto& b) { return a.drift > b.drift; });
        int over1 = 0, overHalf = 0;
        for (auto& row : rows) { over1 += row.drift > 1.0; overHalf += row.drift > 0.5; }
        printf("step %.2f dB (%d steps, loudness table ~%.0f KB): %d shapes drift more than 1 dB, %d more than 0.5 dB. worst:",
               step, count, shapes.size() * count * 11.0 / 1024.0, over1, overHalf);
        for (int i = 0; i < 5; i++) printf(" %s %.2f dB at %g dB drive,", rows[i].name, rows[i].drift, rows[i].driveDb);
        printf("\n");
    }
}
