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


class DebugId(IntEnum):
    IMU = 0
    CONTROL = 1


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


class PacketError(Exception):
    """A binary frame that doesn't match any known packet layout."""


# --------------------------------------------------------------------------
# Incoming packets
# --------------------------------------------------------------------------

_FAST = struct.Struct("<BQ3ff")          # type, timestamp, orientation xyz, rw
_DEBUG_HEADER = struct.Struct("<BB")     # type, debug_id
_IMU = struct.Struct("<9f")              # IMU_data_t: accel, gyro, mag
_CONTROL = struct.Struct("<Q5f")         # control_data_t: timestamp, angle, angle_ref, w, w_ref, pwm_out


def _parse_fast(data: bytes) -> FastTelemetry:
    if len(data) != _FAST.size:
        raise PacketError(f"fast packet: {len(data)} bytes, expected {_FAST.size}")
    _, t, x, y, z, rw = _FAST.unpack(data)
    return FastTelemetry(t, Vec3(x, y, z), rw)


def _parse_imu(data: bytes, offset: int) -> ImuDebug:
    v = _IMU.unpack_from(data, offset)
    return ImuDebug(Vec3(*v[0:3]), Vec3(*v[3:6]), Vec3(*v[6:9]))


def _parse_control(data: bytes, offset: int) -> ControlDebug:
    return ControlDebug(*_CONTROL.unpack_from(data, offset))


# debug_id -> (payload size, parser). To add a debug type, add its id to
# DebugId, a dataclass for it, a struct and a parser function, and an entry here.
_DEBUG_PAYLOADS = {
    DebugId.IMU: (_IMU.size, _parse_imu),
    DebugId.CONTROL: (_CONTROL.size, _parse_control),
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


def parse_packet(data: bytes):
    """Decode one binary frame into a packet object, or raise PacketError."""
    if not data:
        raise PacketError("empty binary frame")
    parser = _PARSERS.get(data[0])
    if parser is None:
        raise PacketError(f"unknown packet type {data[0]}")
    return parser(data)


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
