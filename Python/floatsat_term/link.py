"""The websocket link to the FloatSat: decodes incoming frames, sends commands,
and lets procedures subscribe to packets."""
import asyncio
import contextlib
import time
from typing import Callable, Iterator

import websockets

from .protocol import PacketError, encode_command, hexdump, parse_packet


class Link:
    def __init__(self, ws,
                 on_packet: Callable[[object], None],
                 on_text: Callable[[str], None],
                 on_warning: Callable[[str], None]):
        self._ws = ws
        self._on_packet = on_packet
        self._on_text = on_text
        self._on_warning = on_warning
        self._subscribers: list[tuple[type, asyncio.Queue]] = []
        self._warned: set[str] = set()

    async def send_command(self, text: str):
        await self._ws.send(encode_command(text))   # bytes -> binary frame

    @contextlib.contextmanager
    def subscribe(self, packet_type: type) -> Iterator[asyncio.Queue]:
        """Every packet of `packet_type` received while inside the `with` block
        is also put on the yielded queue, as a (received_at, packet) tuple.
        received_at is time.monotonic() (the same clock as the event loop's
        time()) at the moment the frame arrived."""
        entry = (packet_type, asyncio.Queue())
        self._subscribers.append(entry)
        try:
            yield entry[1]
        finally:
            self._subscribers.remove(entry)

    async def run(self):
        """Receive until the connection closes."""
        try:
            async for message in self._ws:
                if isinstance(message, str):        # text frame -> info
                    self._on_text(message.rstrip("\r\n"))
                    continue
                try:                                # binary frame -> packet
                    packet = parse_packet(message)
                except PacketError as e:
                    # each kind of bad frame is reported once, not at telemetry rate
                    if str(e) not in self._warned:
                        self._warned.add(str(e))
                        self._on_warning(f"{e} (repeats hidden): {hexdump(message)}")
                    continue
                received_at = time.monotonic()
                self._on_packet(packet)
                for packet_type, queue in self._subscribers:
                    if isinstance(packet, packet_type):
                        queue.put_nowait((received_at, packet))
        except websockets.exceptions.ConnectionClosed:
            pass
