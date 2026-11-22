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


class PacketType(IntEnum):
    FAST = 0
    SLOW = 1
    DEBUG = 4
    CAMERA_HEADER = 6
    CAMERA_CHUNK = 7


class DebugId(IntEnum):        # DEBUG_PACKET_ID_* in Shared/fs_packets.h
    NONE = 0
    IMU = 1
    CONTROL = 2
    RTC = 3


class Vec3(NamedTuple):
    x: float
    y: float
    z: float


@dataclass(frozen=True)
class FastTelemetry:            # telemetry_packet_fast_t
    timestamp: int
    orientation: Vec3
    rw: float


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
class CameraHeader:             # camera_packet_header_t
    nchunks: int
    timeout_ms: int             # max time between two consecutive chunks


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

_FAST = struct.Struct("<BQ3ff")          # type, timestamp, orientation xyz, rw (end marker stripped)
_DEBUG_HEADER = struct.Struct("<BB")     # type, debug_id
_IMU = struct.Struct("<Q9f")              # IMU_data_t: timestamp, accel, gyro, mag
_CONTROL = struct.Struct("<Q6f")         # control_data_t: timestamp, angle, angle_ref, w, w_ref, pwm_out, rw_speed
_RTC = struct.Struct("<Q3B")             # floatsat_time_t: timestamp, hours, minutes, seconds


# Camera packets have no end marker. The chunk header is followed by `len`
# bytes of JPEG data (the `uint8_t *data` member is not sent).
_CAMERA_HEADER = struct.Struct("<BBH")   # type, nchunks, timeout_ms
_CAMERA_CHUNK = struct.Struct("<BBH")    # type, index, len (then the data)


def _parse_camera_header(data: bytes) -> CameraHeader:
    if len(data) != _CAMERA_HEADER.size:
        raise PacketError(f"camera header: {len(data)} bytes, expected {_CAMERA_HEADER.size}")
    _, nchunks, timeout_ms = _CAMERA_HEADER.unpack(data)
    return CameraHeader(nchunks, timeout_ms)


def _parse_camera_chunk(data: bytes) -> CameraChunk:
    if len(data) < _CAMERA_CHUNK.size:
        raise PacketError(f"camera chunk: only {len(data)} bytes")
    _, index, length = _CAMERA_CHUNK.unpack_from(data)
    return CameraChunk(index, length, bytes(data[_CAMERA_CHUNK.size:_CAMERA_CHUNK.size + length]))


def _parse_fast(data: bytes) -> FastTelemetry:
    if len(data) != _FAST.size:
        raise PacketError(f"fast packet: {len(data)} bytes, expected {_FAST.size}")
    _, t, x, y, z, rw = _FAST.unpack(data)
    return FastTelemetry(t, Vec3(x, y, z), rw)


def _parse_imu(data: bytes, offset: int) -> ImuDebug:
    v = _IMU.unpack_from(data, offset)
    return ImuDebug(int(v[0]),Vec3(*v[1:4]), Vec3(*v[4:7]), Vec3(*v[7:10]))


def _parse_control(data: bytes, offset: int) -> ControlDebug:
    return ControlDebug(*_CONTROL.unpack_from(data, offset))


def _parse_rtc(data: bytes, offset: int) -> RtcDebug:
    return RtcDebug(*_RTC.unpack_from(data, offset))


# debug_id -> (payload size, parser). To add a debug type, add its id to
# DebugId, a dataclass for it, a struct and a parser function, and an entry here.
_DEBUG_PAYLOADS = {
    DebugId.IMU: (_IMU.size, _parse_imu),
    DebugId.CONTROL: (_CONTROL.size, _parse_control),
    DebugId.RTC: (_RTC.size, _parse_rtc),
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
