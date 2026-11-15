"""Plot a `test control` CSV: angle, angular velocity and PWM against time.

Standalone, so it can also be run by hand on an old recording:
    python floatsat_term/plot_control.py results/control_log_YYYYMMDD_HHMMSS.csv

`test control` launches it as a separate process after saving the CSV, so the
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

    def col(name):
        return [float(r[name]) for r in rows]

    # timestamp is HAL_GetTick() on the FloatSat (ms); plot seconds from start
    t0 = float(rows[0]["timestamp"])
    t = [(float(r["timestamp"]) - t0) / 1000.0 for r in rows]

    fig, (ax_angle, ax_w, ax_pwm) = plt.subplots(3, 1, sharex=True, figsize=(10, 8))
    fig.canvas.manager.set_window_title(path)

    ax_angle.plot(t, col("angle_ref"), "--", label="reference angle")
    ax_angle.plot(t, col("angle"), label="angle")
    ax_angle.set_ylabel("angle")
    ax_angle.set_title("Angle")

    ax_w.plot(t, col("w_ref"), "--", label="reference angular velocity")
    ax_w.plot(t, col("w"), label="angular velocity")
    ax_w.set_ylabel("angular velocity")
    ax_w.set_title("Angular velocity")

    ax_pwm.plot(t, col("pwm_out"), label="PWM")
    ax_pwm.set_ylabel("PWM")
    ax_pwm.set_title("PWM output")
    ax_pwm.set_xlabel("time [s]")

    for ax in (ax_angle, ax_w, ax_pwm):
        ax.grid(True)
        ax.legend(loc="upper right")

    fig.tight_layout()
    plt.show()


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(f"usage: python {sys.argv[0]} <control_log.csv>")
    main(sys.argv[1])
