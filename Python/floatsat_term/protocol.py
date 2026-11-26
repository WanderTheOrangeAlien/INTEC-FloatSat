"""FloatSat wire protocol: packet layouts and command encoding.

Everything here is pure (bytes in, objects out), so it can be unit tested
without a terminal or a websocket.

The C structs are packed and little-endian, so every format uses '<'
(little-endian, no padding).
"""
import struct
from dataclasses import dataclass
from enum import IntEnum
from typing import NamedTuple


class PacketType(IntEnum):    # PACKET_TYPE_* in Shared/fs_packets.h
    FAST = 0
    EXTENDED = 1                # fast + slow in one frame
    DEBUG = 4
    CAMERA_HEADER = 6
    CAMERA_CHUNK = 7


class DebugId(IntEnum):        # DEBUG_PACKET_ID_* in Shared/fs_packets.h
    NONE = 0
    IMU = 1
    CONTROL = 2
    RTC = 3
    LIGHT = 4


class MissionId(IntEnum):      # MISSION_ID_* in Shared/fs_commands.h
    PLAYGROUND = 0
    SUNCHASER = 1
    PHOTO = 2


class SunchaserState(IntEnum): # MISSION_STATE_SUN_* in CubeIDE/App/Inc/floatsat_core_types.h
    SEARCH = 0
    ACQUIRE = 1
    TRACK = 2
    LOST = 3


LIGHT_SENSOR_AMOUNT = 3         # FLOATSAT_LIGHT_SENSOR_AMOUNT in Shared/floatsat_defs.h


class Vec3(NamedTuple):
    x: float
    y: float
    z: float


@dataclass(frozen=True)
class FastTelemetry:            # telemetry_packet_t header + telemetry_packet_fast_t
    timestamp: int
    orientation: Vec3
    angular_vel: float          # deg/s
    rw_speed: float             # rev/s
    theta_ref: float            # deg


class ControlParams(NamedTuple):    # control_cascade_params_t
    k_theta: float
    k_p: float
    k_i: float


@dataclass(frozen=True)
class SlowTelemetry:            # telemetry_packet_slow_t
    current_mission: int        # MissionId, kept as int so unknown ids still show
    control_params: ControlParams
    sunchaser_state: int        # SunchaserState; only meaningful in the sunchaser mission (0 otherwise)
    light_r: float
    light_l: float


@dataclass(frozen=True)
class ExtendedTelemetry(FastTelemetry):     # PACKET_TYPE_EXTENDED: fast fields + slow
    # A subclass of FastTelemetry, so link.subscribe(FastTelemetry) also gets
    # the fast part of extended frames.
    slow: SlowTelemetry


@dataclass(frozen=True)
class ImuDebug:                 # telemetry_packet_debug_t with debug_id = IMU
    timestamp: int
    accel: Vec3
    gyro: Vec3
    mag: Vec3


@dataclass(frozen=True)
class ControlDebug:             # telemetry_packet_debug_t with debug_id = CONTROL
    timestamp: int
    angle: float
    angle_ref: float
    w: float
    w_ref: float
    pwm_out: float
    rw_speed: float


@dataclass(frozen=True)
class RtcDebug:                 # telemetry_packet_debug_t with debug_id = RTC
    timestamp: int
    hours: int
    minutes: int
    seconds: int


@dataclass(frozen=True)
class LightDebug:               # telemetry_packet_debug_t with debug_id = LIGHT
    timestamp: int
    readings: tuple[float, ...] # LIGHT_SENSOR_AMOUNT values, indexed by LIGHT_SENSOR_IDX_*
    e_light: float
    delta_theta: float
    new_theta: float


@dataclass(frozen=True)
class CameraHeader:             # camera_packet_header_t
    nchunks: int
    timeout_ms: int             # max time between two consecutive chunks
    photo_id: int
    hours: int                  # timestamp (floatsat_time_t)
    minutes: int
    seconds: int
    yaw: float
    resolution: int


@dataclass(frozen=True)
class CameraChunk:              # camera_packet_chunk_t
    index: int
    length: int                 # the len field, as sent
    data: bytes                 # the bytes after the chunk header, at most `length`


class PacketError(Exception):
    """A binary frame that doesn't match any known packet layout."""


# --------------------------------------------------------------------------
# Incoming packets
# --------------------------------------------------------------------------

# Every packet ends with a 2-byte end marker (PACKET_END_MARKER_VAL in
# Shared/fs_packets.h). parse_packet checks and strips it, so the structs
# below describe the frame *without* the marker.
END_MARKER = 0xCAFE
_END_MARKER = struct.Struct("<H")

_FAST = struct.Struct("<BQ3f3f")         # type, timestamp, orientation xyz, angular_vel, rw_speed, theta_ref (end marker stripped)
# An extended frame is the whole telemetry_packet_t: the fast frame *with* its
# end marker, then telemetry_packet_slow_t (whose end marker is stripped).
_SLOW = struct.Struct("<B3fB2f")         # current_mission, k_theta, k_p, k_i, sunchaser_state, light_r, light_l
_EXTENDED_SIZE = _FAST.size + _END_MARKER.size + _SLOW.size
_DEBUG_HEADER = struct.Struct("<BB")     # type, debug_id
_IMU = struct.Struct("<Q9f")              # IMU_data_t: timestamp, accel, gyro, mag
_CONTROL = struct.Struct("<Q6f")         # control_data_t: timestamp, angle, angle_ref, w, w_ref, pwm_out, rw_speed
_RTC = struct.Struct("<Q3B")             # floatsat_time_t: timestamp, hours, minutes, seconds
_LIGHT = struct.Struct(f"<Q{LIGHT_SENSOR_AMOUNT}f3f")  # light_control_data_t: timestamp, readings, e_light, delta_theta, new_theta


# Camera packets have no end marker. The chunk header is followed by `len`
# bytes of JPEG data (the `uint8_t *data` member is not sent).
_CAMERA_HEADER = struct.Struct("<BBHB3BfB")  # type, nchunks, timeout_ms, photo_id, hh mm ss, yaw, resolution
_CAMERA_CHUNK = struct.Struct("<BBH")    # type, index, len (then the data)


def _parse_camera_header(data: bytes) -> CameraHeader:
    if len(data) != _CAMERA_HEADER.size:
        raise PacketError(f"camera header: {len(data)} bytes, expected {_CAMERA_HEADER.size}")
    _, *fields = _CAMERA_HEADER.unpack(data)
    return CameraHeader(*fields)


def _parse_camera_chunk(data: bytes) -> CameraChunk:
    if len(data) < _CAMERA_CHUNK.size:
        raise PacketError(f"camera chunk: only {len(data)} bytes")
    _, index, length = _CAMERA_CHUNK.unpack_from(data)
    return CameraChunk(index, length, bytes(data[_CAMERA_CHUNK.size:_CAMERA_CHUNK.size + length]))


def _fast_fields(data: bytes) -> tuple:
    _, t, x, y, z, w, rw, theta_ref = _FAST.unpack_from(data)
    return t, Vec3(x, y, z), w, rw, theta_ref


def _parse_fast(data: bytes) -> FastTelemetry:
    if len(data) != _FAST.size:
        raise PacketError(f"fast packet: {len(data)} bytes, expected {_FAST.size}")
    return FastTelemetry(*_fast_fields(data))


def _parse_extended(data: bytes) -> ExtendedTelemetry:
    if len(data) != _EXTENDED_SIZE:
        raise PacketError(f"extended packet: {len(data)} bytes, expected {_EXTENDED_SIZE}")
    (marker,) = _END_MARKER.unpack_from(data, _FAST.size)
    if marker != END_MARKER:
        raise PacketError(f"extended packet: bad end marker 0x{marker:04x} after the fast part, "
                          f"expected 0x{END_MARKER:04x}")
    mission, k_theta, k_p, k_i, state, light_r, light_l = \
        _SLOW.unpack_from(data, _FAST.size + _END_MARKER.size)
    slow = SlowTelemetry(mission, ControlParams(k_theta, k_p, k_i), state, light_r, light_l)
    return ExtendedTelemetry(*_fast_fields(data), slow)


def _parse_imu(data: bytes, offset: int) -> ImuDebug:
    v = _IMU.unpack_from(data, offset)
    return ImuDebug(int(v[0]),Vec3(*v[1:4]), Vec3(*v[4:7]), Vec3(*v[7:10]))


def _parse_control(data: bytes, offset: int) -> ControlDebug:
    return ControlDebug(*_CONTROL.unpack_from(data, offset))


def _parse_rtc(data: bytes, offset: int) -> RtcDebug:
    return RtcDebug(*_RTC.unpack_from(data, offset))


def _parse_light(data: bytes, offset: int) -> LightDebug:
    t, *v = _LIGHT.unpack_from(data, offset)
    n = LIGHT_SENSOR_AMOUNT
    return LightDebug(t, tuple(v[:n]), *v[n:])


# debug_id -> (payload size, parser). To add a debug type, add its id to
# DebugId, a dataclass for it, a struct and a parser function, and an entry here.
_DEBUG_PAYLOADS = {
    DebugId.IMU: (_IMU.size, _parse_imu),
    DebugId.CONTROL: (_CONTROL.size, _parse_control),
    DebugId.RTC: (_RTC.size, _parse_rtc),
    DebugId.LIGHT: (_LIGHT.size, _parse_light),
}


def _parse_debug(data: bytes):
    if len(data) < _DEBUG_HEADER.size:
        raise PacketError(f"debug packet: only {len(data)} bytes")
    _, debug_id = _DEBUG_HEADER.unpack_from(data)
    entry = _DEBUG_PAYLOADS.get(debug_id)
    if entry is None:
        raise PacketError(f"debug packet: unknown debug_id {debug_id}")
    size, parser = entry
    # The firmware sends sizeof(telemetry_packet_debug_t), which is sized for
    # the union's largest member, so the frame can be longer than this member.
    # (The end marker sits after the whole union and was already stripped.)
    need = _DEBUG_HEADER.size + size
    if len(data) < need:
        raise PacketError(f"debug packet id {debug_id}: {len(data)} bytes, expected at least {need}")
    return parser(data, _DEBUG_HEADER.size)


# packet type -> parser. To add a packet type: add it to PacketType, a
# dataclass for it, and an entry here.
_PARSERS = {
    PacketType.FAST: _parse_fast,
    PacketType.EXTENDED: _parse_extended,
    PacketType.DEBUG: _parse_debug,
}

# packet types that carry no end marker
_NO_END_MARKER = {
    PacketType.CAMERA_HEADER: _parse_camera_header,
    PacketType.CAMERA_CHUNK: _parse_camera_chunk,
}


def parse_packet(data: bytes):
    """Decode one binary frame into a packet object, or raise PacketError."""
    if not data:
        raise PacketError("empty binary frame")
    if data[0] in _NO_END_MARKER:
        return _NO_END_MARKER[data[0]](data)
    parser = _PARSERS.get(data[0])
    if parser is None:
        raise PacketError(f"unknown packet type {data[0]}")
    if len(data) < 1 + _END_MARKER.size:
        raise PacketError(f"packet type {data[0]}: {len(data)} bytes, too short for end marker")
    (marker,) = _END_MARKER.unpack_from(data, len(data) - _END_MARKER.size)
    if marker != END_MARKER:
        raise PacketError(f"packet type {data[0]}: bad end marker 0x{marker:04x}, expected 0x{END_MARKER:04x}")
    return parser(data[:-_END_MARKER.size])


# --------------------------------------------------------------------------
# Outgoing commands
# --------------------------------------------------------------------------

CMD_MAGIC = 0xCAFE
_CMD_HEADER = struct.Struct("<HHH")      # magic, payload length, 0


def encode_command(text: str) -> bytes:
    payload = text.encode()
    return _CMD_HEADER.pack(CMD_MAGIC, len(payload), 0x00) + payload


def hexdump(data: bytes, limit: int = 16) -> str:
    s = ' '.join(f'{b:02x}' for b in data[:limit])
    return s + (f" ... ({len(data)} bytes)" if len(data) > limit else "")
