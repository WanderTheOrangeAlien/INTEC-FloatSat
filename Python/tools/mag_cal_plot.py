#!/usr/bin/env python3
"""
2D magnetometer calibration by algebraic ellipse fitting.

Usage:
    python magcal_plot.py data.csv
    python magcal_plot.py data.csv --delimiter ";"

Reads X from the 2nd column and Y from the 3rd column of the CSV (header rows and
non-numeric rows are skipped), fits the ellipse, prints the calibration and shows
the raw and corrected data side by side.
"""

import argparse
import sys

import numpy as np
import matplotlib.pyplot as plt


def magcal_fit(m):
    """m: (n, 2) raw samples -> (h, C, R) such that |C (m - h)| ~= R."""
    mu = m.mean(0)
    sigma = np.sqrt(((m - mu) ** 2).sum(1).mean())
    x, y = ((m - mu) / sigma).T
    Phi = np.column_stack([x * x - y * y, x * y, x, y, np.ones_like(x)])
    a, b, d, e, f = np.linalg.lstsq(Phi, -y * y, rcond=None)[0]
    c = 1 - a
    den = 4 * a * c - b * b
    if den <= 0:
        raise ValueError("Fitted conic is not an ellipse (4ac - b^2 <= 0).")
    x0, y0 = (b * e - 2 * c * d) / den, (b * d - 2 * a * e) / den
    f0 = f + 0.5 * (d * x0 + e * y0)
    if f0 >= 0:
        raise ValueError("Degenerate ellipse (f0 >= 0).")
    M = np.array([[a, b / 2], [b / 2, c]]) / -f0
    s = np.sqrt(np.linalg.det(M))
    C = (M + s * np.eye(2)) / (np.sqrt(np.trace(M) + 2 * s) * np.sqrt(s))
    h = mu + sigma * np.array([x0, y0])
    return h, C, sigma / np.sqrt(s)


def load_xy(path, delimiter=","):
    data = np.genfromtxt(path, delimiter=delimiter, dtype=float, invalid_raise=False)
    if data.ndim != 2 or data.shape[1] < 3:
        raise ValueError(f"Expected at least 3 columns in '{path}'.")
    xy = data[:, 1:3]
    xy = xy[np.isfinite(xy).all(axis=1)]   # drops header / malformed rows
    if len(xy) < 20:
        raise ValueError(f"Only {len(xy)} valid samples found; need at least 20.")
    return xy


def main():
    parser = argparse.ArgumentParser(description="Fit and plot 2D magnetometer calibration.")
    parser.add_argument("csv", help="CSV file; column 2 = X, column 3 = Y")
    parser.add_argument("--delimiter", default=",", help="field delimiter (default ',')")
    args = parser.parse_args()

    try:
        m = load_xy(args.csv, args.delimiter)
        h, C, R = magcal_fit(m)
    except (OSError, ValueError) as err:
        sys.exit(f"Error: {err}")

    b_cal = (m - h) @ C.T
    r = np.linalg.norm(b_cal, axis=1)
    rms_rel = np.sqrt(np.mean((r - R) ** 2)) / R

    # Ellipse geometry, for reporting
    Craw_inv = np.linalg.inv(C)
    lam, vec = np.linalg.eigh(C.T @ C / R**2)         # M = (C/R)^T (C/R), ascending eigenvalues
    semi = 1 / np.sqrt(lam)                            # semi[0] is the major semi-axis
    tilt = np.degrees(np.arctan2(vec[1, 0], vec[0, 0]))  # direction of the major axis
    tilt = (tilt + 90) % 180 - 90                      # fold into [-90, 90)

    np.set_printoptions(precision=6, suppress=True)
    print(f"Samples          : {len(m)}")
    print(f"Hard iron h      : [{h[0]:.4f}, {h[1]:.4f}]")
    print(f"Soft iron C      :\n{C}")
    print(f"Radius R         : {R:.4f}")
    print(f"Semi-axes        : {semi.max():.4f}, {semi.min():.4f}  (ratio {semi.max() / semi.min():.4f})")
    print(f"Ellipse tilt     : {tilt:.2f} deg")
    print(f"RMS radial error : {100 * rms_rel:.3f} % of R")

    # Curves for overlay
    t = np.linspace(0, 2 * np.pi, 400)
    circle = R * np.column_stack([np.cos(t), np.sin(t)])
    ellipse = circle @ Craw_inv.T + h                 # inverse map of the circle

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 6))

    ax1.scatter(m[:, 0], m[:, 1], s=6, alpha=0.6, label="raw samples")
    ax1.plot(ellipse[:, 0], ellipse[:, 1], "r-", lw=1.5, label="fitted ellipse")
    ax1.plot(*h, "r+", ms=14, mew=2, label="center h")
    ax1.axhline(0, color="gray", lw=0.6)
    ax1.axvline(0, color="gray", lw=0.6)
    ax1.set_title("Original data")
    ax1.set_xlabel("X (raw)")
    ax1.set_ylabel("Y (raw)")

    ax2.scatter(b_cal[:, 0], b_cal[:, 1], s=6, alpha=0.6, color="tab:green", label="corrected samples")
    ax2.plot(circle[:, 0], circle[:, 1], "r-", lw=1.5, label=f"circle R = {R:.2f}")
    ax2.plot(0, 0, "r+", ms=14, mew=2)
    ax2.axhline(0, color="gray", lw=0.6)
    ax2.axvline(0, color="gray", lw=0.6)
    ax2.set_title(f"Transformed data  (RMS radial error {100 * rms_rel:.2f} %)")
    ax2.set_xlabel("X (calibrated)")
    ax2.set_ylabel("Y (calibrated)")

    for ax in (ax1, ax2):
        ax.set_aspect("equal", adjustable="datalim")
        ax.grid(True, alpha=0.3)
        ax.legend(loc="upper right", fontsize=8)

    fig.tight_layout()
    plt.show()


if __name__ == "__main__":
    main()