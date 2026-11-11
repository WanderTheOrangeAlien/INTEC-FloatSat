"""How each packet is shown on the fixed rows at the top of the console.

The row assignments below are the whole screen layout. The console puts the
prompt right after STATUS_ROWS and the log below it.
"""
from .console import Console
from .protocol import ControlDebug, FastTelemetry, ImuDebug, Vec3

ROW_FAST = 1        # 1 row
ROW_IMU = 2         # 3 rows (accel, gyro, mag)
ROW_CONTROL = 5     # 1 row
STATUS_ROWS = 5     # number of fixed rows above the prompt (the last row used)


def _vec(label: str, v: Vec3) -> str:
    # fixed width with room for the sign, so the columns don't jitter
    return f"{label:<6} x {v.x:>10.4f}   y {v.y:>10.4f}   z {v.z:>10.4f}"


def show_packet(console: Console, packet):
    match packet:
        case FastTelemetry():
            o = packet.orientation
            console.status(ROW_FAST,
                           f"({packet.timestamp}) Orientation: "
                           f"{{{o.x:.2f} {o.y:.2f} {o.z:.2f}}} RW: {packet.rw:.2f}")

        case ImuDebug():
            console.status(ROW_IMU,
                           _vec("accel", packet.accel),
                           _vec("gyro", packet.gyro),
                           _vec("mag", packet.mag))

        case ControlDebug():
            # each "a -> b" reads as measured value -> reference
            console.status(ROW_CONTROL,
                           f"({packet.timestamp}) ctl  "
                           f"angle {packet.angle:>8.2f} -> {packet.angle_ref:>8.2f}  "
                           f"w {packet.w:>8.3f} -> {packet.w_ref:>8.3f}  "
                           f"pwm {packet.pwm_out:>6.3f}  "
                           f"rw {packet.rw_speed:>8.2f}")
