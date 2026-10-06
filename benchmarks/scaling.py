#!/usr/bin/env python3
"""Thread-scaling benchmark for kessler_screen.

Runs the fast screener on the same input with 1, 2, 4, ... threads, saves the
timings to a CSV file and draws a graph (if matplotlib is installed).

Usage:
    python3 benchmarks/scaling.py data/active.tle --hours 2
Options:
    --binary   path to kessler_screen (default: build-release/kessler_screen)
    --threads  comma-separated thread counts (default: 1,2,4,8,12,16, capped at your CPU count)
    --repeats  runs per thread count; the fastest is kept (default: 1)
Output:
    benchmarks/results/scaling.csv and benchmarks/results/scaling.png
"""
import argparse
import csv
import os
import re
import subprocess
import sys

# Lines printed by kessler_screen that we read, e.g. "  grid setup   14.29 s  (one thread)".
FIELDS = ["grid", "propagation", "grid setup", "pair search", "merge"]


def run_once(binary, catalog, hours, threads):
    env = dict(os.environ, OMP_NUM_THREADS=str(threads))
    result = subprocess.run([binary, catalog, str(hours)], env=env, capture_output=True,
                            text=True, check=True)
    row = {"threads": threads}
    for field in FIELDS:
        match = re.search(rf"^\s*{re.escape(field)}\s+([0-9.]+) s", result.stdout, re.MULTILINE)
        if match is None:
            sys.exit(f"could not find '{field}' in the output of {binary}")
        row[field] = float(match.group(1))
    return row


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("catalog")
    parser.add_argument("--hours", type=float, default=2.0)
    parser.add_argument("--binary", default="build-release/kessler_screen")
    parser.add_argument("--threads", default="1,2,4,8,12,16")
    parser.add_argument("--repeats", type=int, default=1)
    args = parser.parse_args()

    cpus = os.cpu_count() or 1
    thread_counts = sorted({t for t in map(int, args.threads.split(",")) if 1 <= t <= cpus})
    os.makedirs("benchmarks/results", exist_ok=True)

    rows = []
    for t in thread_counts:
        best = min((run_once(args.binary, args.catalog, args.hours, t) for _ in range(args.repeats)),
                   key=lambda r: r["grid"])
        rows.append(best)
        print(f"threads={t:2d}  grid {best['grid']:7.2f} s  "
              f"(propagation {best['propagation']:.2f}, setup {best['grid setup']:.2f}, "
              f"search {best['pair search']:.2f}, merge {best['merge']:.2f})")

    base = rows[0]["grid"]
    for r in rows:
        r["speedup"] = base / r["grid"]

    csv_path = "benchmarks/results/scaling.csv"
    with open(csv_path, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=["threads", *FIELDS, "speedup"])
        writer.writeheader()
        writer.writerows(rows)
    print(f"wrote {csv_path}")

    try:
        import matplotlib
        matplotlib.use("Agg")  # draw to a file, no window needed
        import matplotlib.pyplot as plt
    except ImportError:
        print("matplotlib not installed: skipping the graph (pip install matplotlib)")
        return

    threads = [r["threads"] for r in rows]
    fig, (left, right) = plt.subplots(1, 2, figsize=(11, 4.2))

    left.plot(threads, threads, "--", color="gray", label="perfect scaling")
    left.plot(threads, [r["speedup"] for r in rows], "o-", label="measured")
    left.set_xlabel("threads")
    left.set_ylabel(f"speedup vs 1 thread")
    left.set_title("Fast screener: speedup")
    left.legend()
    left.grid(alpha=0.3)

    bottom = [0.0] * len(rows)
    parts = [("propagation", "propagation (all threads)"), ("pair search", "pair search (all threads)"),
             ("grid setup", "grid setup (one thread)"), ("merge", "merge (one thread)")]
    labels = [str(t) for t in threads]
    for key, label in parts:
        values = [r[key] for r in rows]
        right.bar(labels, values, bottom=bottom, label=label)
        bottom = [b + v for b, v in zip(bottom, values)]
    right.set_xlabel("threads")
    right.set_ylabel("seconds")
    right.set_title("Where the time goes")
    right.legend(fontsize=8)

    fig.suptitle(f"kessler: {args.catalog}, {args.hours:g} h window")
    fig.tight_layout()
    png_path = "benchmarks/results/scaling.png"
    fig.savefig(png_path, dpi=150)
    print(f"wrote {png_path}")


if __name__ == "__main__":
    main()
