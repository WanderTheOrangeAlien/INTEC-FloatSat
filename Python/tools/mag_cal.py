#!/usr/bin/env python3
"""Hard/soft-iron calibration for the FloatSat magnetometer (X/Y plane).

Usage:
    python mag_cal.py mag_log.csv [--xcol mag_x] [--ycol mag_y] [--plot]

Fits a general ellipse  A x^2 + B xy + C y^2 + D x + E y = 1  to the samples
(least squares), then reports two corrections:

1. Axis-aligned (offset + per-axis scale):
       x_cal = (x - off_x) * scale_x
       y_cal = (y - off_y) * scale_y
   Exact only if the ellipse is not tilted. Cheap: 2 subs + 2 muls.

2. Full 2x2 soft-iron matrix:
       [x_cal, y_cal]^T = W @ [x - off_x, y - off_y]^T
   Exact for any tilt. W maps the ellipse onto a circle whose radius is the
   mean radius of the original ellipse, so the output stays in gauss.

Each correction is scored by how round the result is (std of the radius) and
by the worst heading error it would leave (in degrees).
"""
import argparse
import csv
import sys

import numpy as np


def load(path, xcol, ycol):
    with open(path, newline="") as f:
        rows = list(csv.DictReader(f))
    if not rows:
        sys.exit(f"{path}: no data rows")
    for col in (xcol, ycol):
        if col not in rows[0]:
            sys.exit(f"{path}: column '{col}' not found (have {list(rows[0])})")
    x = np.array([float(r[xcol]) for r in rows])
    y = np.array([float(r[ycol]) for r in rows])
    return x, y


def fit_ellipse(x, y):
    """Least-squares conic fit. Returns center, semi-axes, axis directions."""
    M = np.column_stack([x * x, x * y, y * y, x, y])
    A, B, C, D, E = np.linalg.lstsq(M, np.ones_like(x), rcond=None)[0]

    if B * B - 4 * A * C >= 0:
        sys.exit("Fit is not an ellipse. Did the sweep cover a full turn?")

    # Center: gradient of the conic is zero
    center = np.linalg.solve([[2 * A, B], [B, 2 * C]], [-D, -E])

    # Shift to center: [x y] Q [x y]^T = k
    Q = np.array([[A, B / 2], [B / 2, C]])
    k = 1 + A * center[0] ** 2 + B * center[0] * center[1] + C * center[1] ** 2
    evals, evecs = np.linalg.eigh(Q)        # columns of evecs = principal axes
    semi_axes = np.sqrt(k / evals)
    return center, semi_axes, evecs


def score(xc, yc):
    """Roundness of a corrected point cloud: mean radius, std, std in %."""
    r = np.hypot(xc, yc)
    return r.mean(), r.std(), 100.0 * r.std() / r.mean()


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("csv")
    ap.add_argument("--xcol", default="mag_x")
    ap.add_argument("--ycol", default="mag_y")
    ap.add_argument("--plot", action="store_true", help="show raw vs corrected")
    args = ap.parse_args()

    x, y = load(args.csv, args.xcol, args.ycol)
    if len(x) < 20:
        sys.exit("Too few samples for a reliable fit")

    center, semi, V = fit_ellipse(x, y)
    off_x, off_y = center
    r_ref = semi.mean()                     # keep output in the input's units

    # Coverage check: a partial arc gives a badly conditioned fit
    ang = np.unwrap(np.arctan2(y - off_y, x - off_x))
    coverage = np.degrees(ang.max() - ang.min())

    # ---- 1. Axis-aligned correction ----
    # Extent of the ellipse along X and Y (exact, from the fitted shape,
    # not from noisy min/max of the samples)
    ext_x = np.sqrt((semi[0] * V[0, 0]) ** 2 + (semi[1] * V[0, 1]) ** 2)
    ext_y = np.sqrt((semi[0] * V[1, 0]) ** 2 + (semi[1] * V[1, 1]) ** 2)
    scale_x = r_ref / ext_x
    scale_y = r_ref / ext_y
    xa = (x - off_x) * scale_x
    ya = (y - off_y) * scale_y

    # ---- 2. Full soft-iron matrix ----
    W = V @ np.diag(r_ref / semi) @ V.T
    pts = np.column_stack([x - off_x, y - off_y]) @ W.T
    xm, ym = pts[:, 0], pts[:, 1]

    tilt = np.degrees(np.arctan2(V[1, 1], V[0, 1])) % 180   # major axis angle

    print(f"File: {args.csv}  ({len(x)} samples)")
    print(f"Angular coverage around center: {coverage:.0f} deg"
          + ("" if coverage >= 330 else "   <-- less than a full turn, fit may be poor"))
    print()
    print(f"Hard-iron offset:   off_x = {off_x:+.5f}   off_y = {off_y:+.5f}")
    print(f"Ellipse semi-axes:  {semi.min():.5f} / {semi.max():.5f}"
          f"   (ratio {semi.max() / semi.min():.3f}, major axis at {tilt:.0f} deg)")
    print(f"Offset / radius:    {np.hypot(off_x, off_y) / r_ref:.2f}"
          "  (>1 means raw heading can never cover 360 deg)")
    print()

    print("Correction 1: offset + per-axis scale")
    print(f"  scale_x = {scale_x:.5f}   scale_y = {scale_y:.5f}")
    R, s, h = score(xa, ya)
    print(f"  radius {R:.4f} +- {s:.4f}  ({h:.1f} % of radius)")
    print()

    print("Correction 2: offset + 2x2 soft-iron matrix")
    print(f"  W = [[{W[0, 0]:+.5f}, {W[0, 1]:+.5f}],")
    print(f"       [{W[1, 0]:+.5f}, {W[1, 1]:+.5f}]]")
    R, s, h = score(xm, ym)
    print(f"  radius {R:.4f} +- {s:.4f}  ({h:.1f} % of radius)")
    print()

    print("C (paste into the IMU calibration data):")
    print(f"  .mag_offset = {{ .x = {off_x:.6f}f, .y = {off_y:.6f}f }},")
    print(f"  .mag_scale  = {{ .x = {scale_x:.6f}f, .y = {scale_y:.6f}f }},")
    print(f"  .mag_soft_iron = {{ {{ {W[0, 0]:.6f}f, {W[0, 1]:.6f}f }},"
          f" {{ {W[1, 0]:.6f}f, {W[1, 1]:.6f}f }} }},")

    if args.plot:
        import matplotlib.pyplot as plt
        th = np.linspace(0, 2 * np.pi, 200)
        fig, ax = plt.subplots(figsize=(6, 6))
        ax.plot(x, y, ".", ms=3, label="raw")
        ax.plot(xa, ya, ".", ms=3, label="offset + scale")
        ax.plot(xm, ym, ".", ms=3, label="offset + matrix")
        ax.plot(r_ref * np.cos(th), r_ref * np.sin(th), "k--", lw=1, label="target circle")
        ax.plot(0, 0, "k+")
        ax.set_aspect("equal")
        ax.grid(True)
        ax.set_xlabel(args.xcol)
        ax.set_ylabel(args.ycol)
        ax.legend()
        plt.show()


if __name__ == "__main__":
    main()
