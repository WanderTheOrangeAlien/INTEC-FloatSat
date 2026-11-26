"""How each packet is shown on the fixed rows at the top of the console.

The row assignments below are the whole screen layout. The console puts the
prompt right after STATUS_ROWS and the log below it.
"""
from .console import Console
from .protocol import (ControlDebug, ExtendedTelemetry, FastTelemetry, ImuDebug, LightDebug,
                       MissionId, RtcDebug, SlowTelemetry, SunchaserState, Vec3)

ROW_FAST = 1        # 1 row
ROW_SLOW = 2        # 1 row
ROW_IMU = 3         # 3 rows (accel, gyro, mag)
ROW_CONTROL = 6     # 1 row
ROW_LIGHT = 7       # 1 row
ROW_RTC = 8         # 1 row
STATUS_ROWS = 8     # number of fixed rows above the prompt (the last row used)


def _vec(label: str, v: Vec3) -> str:
    # fixed width with room for the sign, so the columns don't jitter
    return f"{label:<6} x {v.x:>10.4f}   y {v.y:>10.4f}   z {v.z:>10.4f}"


def _enum_name(enum, value: int) -> str:
    try:
        return enum(value).name.lower()
    except ValueError:
        return f"?{value}"          # an id the console doesn't know yet


def _show_fast(console: Console, packet: FastTelemetry):
    o = packet.orientation
    console.status(ROW_FAST,
                   f"({packet.timestamp}) fast "
                   f"orient {{{o.x:>6.2f} {o.y:>6.2f} {o.z:>6.2f}}}  "
                   f"w {packet.angular_vel:>8.2f}  "
                   f"rw {packet.rw_speed:>7.2f}  "
                   f"theta_ref {packet.theta_ref:>7.2f}")


def _show_slow(console: Console, timestamp: int, slow: SlowTelemetry):
    p = slow.control_params
    line = (f"({timestamp}) slow "
            f"mission {_enum_name(MissionId, slow.current_mission):<10}  "
            f"k_theta {p.k_theta:.3f} k_p {p.k_p:.3f} k_i {p.k_i:.3f}")
    # the firmware only fills these in the sunchaser mission (zeros otherwise)
    if slow.current_mission == MissionId.SUNCHASER:
        line += (f"  sun {_enum_name(SunchaserState, slow.sunchaser_state):<7}  "
                 f"light L {slow.light_l:>8.2f} R {slow.light_r:>8.2f}")
    console.status(ROW_SLOW, line)


def show_packet(console: Console, packet):
    match packet:
        case ExtendedTelemetry():           # before FastTelemetry: it is a subclass
            _show_fast(console, packet)
            _show_slow(console, packet.timestamp, packet.slow)

        case FastTelemetry():
            _show_fast(console, packet)

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

        case LightDebug():
            readings = " ".join(f"{r:>8.2f}" for r in packet.readings)
            console.status(ROW_LIGHT,
                           f"({packet.timestamp}) light "
                           f"[{readings}]  "
                           f"e {packet.e_light:>7.4f}  "
                           f"d_theta {packet.delta_theta:>7.4f}  "
                           f"new_theta {packet.new_theta:>8.2f}")

        case RtcDebug():
            console.status(ROW_RTC,
                           f"({packet.timestamp}) rtc  "
                           f"{packet.hours:02d}:{packet.minutes:02d}:{packet.seconds:02d}")
