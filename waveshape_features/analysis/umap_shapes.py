"""Maps every waveshape to 2D with UMAP, from the harmonics harmonic_features measured, and plots the map
coloured by brightness.

The harmonics (in dB, one block per drive) are clipped at a floor first, so differences down in the noise
don't pull shapes apart, then UMAP places shapes with similar spectra near each other.

UMAP alone leaves big empty stretches: the shapes form a long smooth-to-noisy arc plus a separate blob of
the asymmetric ones, and no setting fills a square with that. --uniform spreads the finished map over a unit
square for an XY pad:
    rank   each axis replaced by its rank, so both axes are evenly spread and every shape keeps its
           left/right and up/down order. the two together can still bunch up along a diagonal
    grid   every shape gets its own cell of a near-square grid, picked so shapes move as little as possible
           in total. perfectly even, but the gaps have to close somewhere, so a few unlike shapes end up adjacent
Either way it prints how many of each shape's 10 nearest neighbours survive, against the UMAP map and against
the harmonics themselves.

    python3 waveshape_features/analysis/umap_shapes.py [build/harmonic_features.csv] [--out build/waveshape_umap]
        [--uniform none|rank|grid] [--header dsp/Distortions/waveshape/WaveshapeMap.h]
        [--floor -100] [--neighbors 30] [--min-dist 0.99] [--seed 42]

Writes <out>.csv -- name, group, x, y (0 .. 1 with --uniform), and brightness / flatness / oddness averaged
over the drives -- and <out>.png. --header also writes the map the plugin's XY pad plays from, positions
scaled to 0 .. 1 and then shrunk evenly around the middle until every shape sits inside the pad's disc. Needs numpy, matplotlib, scipy and umap-learn (python3 -m pip install umap-learn).
"""

import argparse
import csv
import re
import sys
import warnings

import numpy as np


def neighbours(points, k=10):
    distances = np.linalg.norm(points[:, None] - points[None], axis=2)
    np.fill_diagonal(distances, np.inf)
    return np.argsort(distances, axis=1)[:, :k]


def neighbours_kept(before, after):
    return np.mean([len(set(a) & set(b)) / len(a) for a, b in zip(before, after)])


def empty_cells(points, side=10):
    unit = (points - points.min(axis=0)) / (points.max(axis=0) - points.min(axis=0))
    occupied = {tuple(cell) for cell in np.minimum((unit * side).astype(int), side - 1)}
    return 1.0 - len(occupied) / side ** 2


def rank_axes(unit):
    return (np.argsort(np.argsort(unit, axis=0), axis=0) + 0.5) / len(unit)


def grid_assign(unit):
    """Every shape to its own cell of the smallest near-square grid that fits them all, keeping the total squared
    distance shapes move as small as possible. Returns cell centres in 0 .. 1."""
    from scipy.optimize import linear_sum_assignment

    count = len(unit)
    cols = int(np.ceil(np.sqrt(count)))
    rows = int(np.ceil(count / cols))
    cells = np.array([[(c + 0.5) / cols, (r + 0.5) / rows] for r in range(rows) for c in range(cols)])
    cost = ((unit[:, None, :] - cells[None, :, :]) ** 2).sum(axis=2)
    shape_index, cell_index = linear_sum_assignment(cost)

    placed = np.empty_like(unit)
    placed[shape_index] = cells[cell_index]
    return placed


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("features", nargs="?", default="build/harmonic_features.csv")
    parser.add_argument("--out", default="build/waveshape_umap")
    parser.add_argument("--uniform", choices=["none", "rank", "grid"], default="none",
                        help="spread the map evenly over a unit square, see above")
    parser.add_argument("--header", help="also write the plugin's map here, e.g. dsp/Distortions/waveshape/WaveshapeMap.h")
    parser.add_argument("--floor", type=float, default=-100.0, help="harmonics below this many dB count as this")
    parser.add_argument("--neighbors", type=int, default=30, help="UMAP n_neighbors: higher keeps more of the global layout")
    parser.add_argument("--min-dist", type=float, default=0.99, help="UMAP min_dist: lower packs clusters tighter")
    parser.add_argument("--seed", type=int, default=42, help="fixed so the same features give the same map")
    args = parser.parse_args()

    try:
        import umap
    except ImportError:
        sys.exit("needs umap-learn: python3 -m pip install umap-learn")

    # a fixed seed makes UMAP run single threaded, which it warns about every time
    warnings.filterwarnings("ignore", message="n_jobs value .* overridden", category=UserWarning)

    with open(args.features, newline="") as f:
        table = list(csv.reader(f))
    header, rows = table[0], table[1:]

    names = [row[0] for row in rows]
    groups = [row[1] for row in rows]
    indices = [int(row[header.index("index")]) for row in rows]

    def columns(pattern):
        picked = [i for i, h in enumerate(header) if re.fullmatch(pattern, h)]
        return np.array([[float(row[i]) for i in picked] for row in rows])

    spectra = np.maximum(columns(r"h\d+_\d+"), args.floor)
    brightness = columns(r"brightness_\d+").mean(axis=1)
    flatness = columns(r"flatness_\d+").mean(axis=1)
    oddness = columns(r"oddness_\d+").mean(axis=1)

    mapped = umap.UMAP(n_neighbors=args.neighbors, min_dist=args.min_dist, random_state=args.seed).fit_transform(spectra)
    xy = mapped

    if args.uniform != "none":
        unit = (mapped - mapped.min(axis=0)) / (mapped.max(axis=0) - mapped.min(axis=0))
        xy = rank_axes(unit) if args.uniform == "rank" else grid_assign(unit)

        harmonic_neighbours, map_neighbours, final_neighbours = neighbours(spectra), neighbours(mapped), neighbours(xy)
        print(f"empty cells of a 10x10 grid: {empty_cells(mapped):.0%} on the UMAP map, {empty_cells(xy):.0%} after {args.uniform}")
        print(f"10 nearest neighbours kept: {neighbours_kept(map_neighbours, final_neighbours):.0%} of the UMAP map's, "
              f"{neighbours_kept(harmonic_neighbours, final_neighbours):.0%} of the harmonics' "
              f"(the UMAP map itself keeps {neighbours_kept(harmonic_neighbours, map_neighbours):.0%})")

    with open(args.out + ".csv", "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["name", "group", "x", "y", "brightness", "flatness", "oddness"])
        for i, name in enumerate(names):
            writer.writerow([name, groups[i], f"{xy[i, 0]:.6f}", f"{xy[i, 1]:.6f}",
                             f"{brightness[i]:.6g}", f"{flatness[i]:.6g}", f"{oddness[i]:.6g}"])

    if args.header:
        unit = xy if args.uniform != "none" else (xy - xy.min(axis=0)) / (xy.max(axis=0) - xy.min(axis=0))

        # the pad keeps its handle inside a disc of radius 0.5, so a shape outside it could never be reached exactly.
        # shrinking the whole map evenly keeps every shape where it was relative to the others
        furthest = np.hypot(unit[:, 0] - 0.5, unit[:, 1] - 0.5).max()
        if furthest > 0.5:
            unit = 0.5 + (unit - 0.5) * (0.5 / furthest)
            print(f"shrunk the map by {1 - 0.5 / furthest:.1%} to fit the pad's disc")
        with open(args.header, "w") as f:
            f.write("#pragma once\n\n"
                    "// generated by waveshape_features/analysis/umap_shapes.py, don't edit. from the repo root, after harmonic_features:\n"
                    f"//   python3 waveshape_features/analysis/umap_shapes.py --uniform {args.uniform} --header {args.header}\n"
                    f"// n_neighbors {args.neighbors}, min_dist {args.min_dist}, seed {args.seed}, floor {args.floor} dB\n\n"
                    "#include \"Waveshapes.h\"\n\n"
                    "namespace waveshapes {\n"
                    "    // every shape's table index, name, and place on the XY pad, 0 .. 1 on both axes. see mixAt() in Waveshapes.h\n"
                    "    inline constexpr MapPoint mapPoints[] = {\n")
            for i, name in enumerate(names):
                f.write(f'        {{ {indices[i]}, "{name}", {unit[i, 0]:.6f}, {unit[i, 1]:.6f} }},\n')
            f.write("    };\n}\n")
        print(f"wrote {args.header}")

    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    markers = ["o", "s", "^", "D", "v", "P"]
    fig, ax = plt.subplots(figsize=(14, 11))
    colour = np.log2(np.maximum(brightness, 1.0))
    for g, group in enumerate(sorted(set(groups), key=groups.index)):
        picked = [i for i, name in enumerate(groups) if name == group]
        points = ax.scatter(xy[picked, 0], xy[picked, 1], c=colour[picked], cmap="viridis",
                            vmin=colour.min(), vmax=colour.max(), marker=markers[g % len(markers)],
                            s=46, edgecolors="black", linewidths=0.3, label=group)
    for i, name in enumerate(names):
        ax.annotate(name, xy[i], fontsize=5, xytext=(3, 2), textcoords="offset points", alpha=0.8)

    ax.legend(loc="best", fontsize=9)
    spread = "" if args.uniform == "none" else f", spread by {args.uniform}"
    ax.set_title(f"waveshapes by harmonic content (UMAP{spread}), coloured by brightness")
    if args.uniform != "none":
        ax.set_aspect("equal")
    ax.set_xticks([])
    ax.set_yticks([])
    fig.colorbar(points, ax=ax, label="brightness: log2 of the power weighted mean harmonic")
    fig.tight_layout()
    fig.savefig(args.out + ".png", dpi=200)

    print(f"wrote {args.out}.csv and {args.out}.png")


if __name__ == "__main__":
    main()
