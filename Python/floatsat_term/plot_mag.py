"""Plot a `test mag` CSV: scatter of mag_y against mag_x.

Standalone, so it can also be run by hand on an old recording:
    python floatsat_term/plot_mag.py results/mag_log_YYYYMMDD_HHMMSS.csv

`test mag` launches it as a separate process after saving the CSV, so the
plot window doesn't block the console.
"""
import csv
import sys

import matplotlib.pyplot as plt


def main(path: str):
    with open(path, newline="") as f:
        rows = list(csv.DictReader(f))
    if not rows:
        sys.exit(f"{path}: no data rows")

    x = [float(r["mag_x"]) for r in rows]
    y = [float(r["mag_y"]) for r in rows]

    fig, ax = plt.subplots(figsize=(8, 8))
    fig.canvas.manager.set_window_title(path)

    ax.scatter(x, y, s=8)
    ax.set_xlabel("mag_x")
    ax.set_ylabel("mag_y")
    ax.set_title("Magnetometer X vs Y")
    ax.set_aspect("equal", adjustable="datalim")   # keep circles circular
    ax.grid(True)

    fig.tight_layout()
    plt.show()


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(f"usage: python {sys.argv[0]} <mag_log.csv>")
    main(sys.argv[1])
