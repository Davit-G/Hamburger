# waveshape_features

The analysis tooling behind the WAVESHAPE distortion type. The shapes live in
`dsp/Distortions/waveshape/Waveshapes.h`, shared with the plugin. This tool analyses them, orders each
group smooth to bright, works out the loudness table, and generates the headers the plugin plays from:
`dsp/Distortions/waveshape/WaveshapeOrder.h` (the order and loudness table) and, through
`analysis/umap_shapes.py`, `dsp/Distortions/waveshape/WaveshapeMap.h` (where each shape sits on the XY pad).

## Build

From the repo root:

```bash
mkdir -p build && clang++ -std=c++17 -O2 waveshape_features/features.cpp -lcurses -o build/wf
```

or through CMake, where every target here is left out of the default build. That binary lands in
`build/waveshape_features/waveshape_features`, and works the same as `build/wf` below:

```bash
cmake --build build --target waveshape_features
```

## Commands

| Command | What it does |
|---|---|
| `build/wf` | Test suite. Analyses every shape, prints each group's order (stdout) and stats (stderr), and runs every check below. Exits 1 on the first failure. |
| `build/wf --header > dsp/Distortions/waveshape/WaveshapeOrder.h` | Regenerates the order and the loudness table. Run it, then the suite. |
| `build/wf --tui` | Plots each shape. Left/right: shape, `,` `.`: group, up/down: zoom, `q`: quit. |
| `build/wf --speed` | Per-sample cost of every shape, slowest first. |

## Adding a shape

1. Write it in `Waveshapes.h` as `inline double name(double x, double s)`, for x in [-1, 1].
2. Add `SHAPE(name, s, Group)` to the `shapes` table at the tuned `s`.
3. Regenerate the order and loudness table, then the harmonics and the map (it needs the loudness table), then
   run the suite:

   ```bash
   build/wf --header > dsp/Distortions/waveshape/WaveshapeOrder.h
   build/harmonic_features build/harmonic_features.csv
   python3 waveshape_features/analysis/umap_shapes.py --header dsp/Distortions/waveshape/WaveshapeMap.h
   build/wf
   ```

4. Rebuild the plugin.

The suite fails on either header no longer matching the table, so step 3 can't be skipped quietly. UMAP isn't
exactly stable, so adding a shape moves the others around the pad a little, and presets land on slightly
different blends.

## Reordering

The order is worked out, not written by hand. The knobs are at the top of `features.cpp`:

| Setting | Effect |
|---|---|
| `flatnessWeight` | 0 orders purely by how curves look; higher follows spectral flatness (smooth → bright) more closely. |
| `slopeWeight` | How much curve slope counts next to curve values when comparing shapes. |
| `outlierSigma` | How far off its neighbours a shape has to be before it gets re-placed. 0 disables. |
| `analysisLevelDb` | Level of the analysis sine, also used for the loudness table and the pad's re-levelling. Lives in `Waveshapes.h`, since the plugin uses it too. |

Change one, regenerate the header, run the suite. Its stderr shows each group's rank correlation with
flatness (1 = pure flatness sort) and the largest step between neighbours.

## What the suite checks

- Every normalised shape stays finite and within ±1.5 over the whole input range.
- `WaveshapeOrder.h` matches the table: every shape in its group once, names and indices agreeing.
- The morph lands exactly on each shape at whole positions.
- The smoothing table matches the morph unsmoothed, stays bounded smoothed, is centred, rounds corners off, and
  grows continuously from the first nudge of the knob.
- The asym skew stays finite for huge inputs.
- The loudness gains are current, and no shape drifts more than 1 dB off target between drive steps.
- `WaveshapeMap.h` holds every shape once inside the pad's disc, a dot on the pad gives exactly its shape, the
  pad's weights always sum to 1, the blend doesn't jump where a shape joins or leaves the nearest few, and
  re-levelling stays between 1 and 2. It also reports how far re-levelled blends still sit below their shapes.

It also prints the level jumps between neighbouring shapes at a few drives. That report is informational;
it doesn't fail the suite.

## Analysis tools

In `analysis/`. Each includes `features.cpp`, so it uses the same measurements. Build them the same way,
e.g. `clang++ -std=c++17 -O2 waveshape_features/analysis/loudness_drift.cpp -lcurses -o build/loudness_drift`,
or `cmake --build build --target waveshape_loudness_drift`.

| Tool | Use it for |
|---|---|
| `table_resolution` | How closely the smoothing table follows the exact curves, per shape, at a few smooth settings. Run after adding jumpy shapes. |
| `loudness_drift [step dB ...]` | Loudness drift between drive steps for candidate step sizes. For picking `driveStepDb`. |
| `loudness_bias_asym` | How far the bias and asym knobs pull loudness off the table, which is measured with both at 0. |
| `asym_strength [k ...]` | Even harmonics and output change of full asym at different `maxAsymK`. |
| `plugin_cost` | Per-sample and table-rebuild cost per group, at each group's slowest spot. |
| `harmonic_features [out.csv]` | Harmonics of every shape at a few drives, loudness gain applied, as a CSV for `umap_shapes.py`. |

## 2D map (UMAP)

`umap_shapes.py` places every shape on a 2D map from its harmonics, so shapes that sound alike sit together,
and plots it coloured by brightness. It needs `umap-learn` (`python3 -m pip install umap-learn`).

```bash
clang++ -std=c++17 -O2 waveshape_features/analysis/harmonic_features.cpp -lcurses -o build/harmonic_features
build/harmonic_features build/harmonic_features.csv
python3 waveshape_features/analysis/umap_shapes.py build/harmonic_features.csv --out build/waveshape_umap
```

That writes `build/waveshape_umap.csv` (name, group, x, y, brightness, flatness, oddness) and
`build/waveshape_umap.png`. Add `--header dsp/Distortions/waveshape/WaveshapeMap.h` to update the plugin's XY pad,
which plays the raw UMAP map, scaled to fill the square. `--neighbors` and `--min-dist` change how UMAP trades local clusters against the
overall layout; the defaults, 30 and 0.99, filled a square best in a sweep without losing neighbours. `--floor`
sets the dB below which harmonic differences are ignored.

Even at its best UMAP leaves empty stretches, so for an XY pad `--uniform` spreads the finished map over a unit
square. `rank` replaces each axis with its rank, keeping every shape's left/right and up/down order. `grid` gives
every shape its own cell, moving shapes as little as possible in total. Both print how many nearest neighbours
survived, so the two can be compared:

```bash
python3 waveshape_features/analysis/umap_shapes.py --uniform rank --out build/waveshape_umap_rank
python3 waveshape_features/analysis/umap_shapes.py --uniform grid --out build/waveshape_umap_grid
```
