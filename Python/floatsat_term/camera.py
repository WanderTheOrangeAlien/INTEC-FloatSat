"""Reassembles camera images sent as one CameraHeader followed by CameraChunks.

  - a CameraHeader starts a new image: it says how many chunks to expect and
    the longest wait allowed between two consecutive chunks (timeout_ms)
  - each CameraChunk is stored by its index; when every index 0..nchunks-1
    has arrived, the chunks are joined in order and saved as a .jpg
  - if no chunk arrives within timeout_ms (counted from the header, then
    from the last chunk), the image is dropped
"""
import asyncio
from datetime import datetime
from pathlib import Path
from typing import Callable

from .protocol import CameraChunk, CameraHeader

IMAGES_DIR = Path("results") / "images"
NAME = "[camera]"


class CameraReceiver:
    def __init__(self, log: Callable[[str], None], images_dir: Path = IMAGES_DIR):
        self._log = log
        self._dir = images_dir
        self._header: CameraHeader | None = None
        self._chunks: dict[int, bytes] = {}
        self._problems: list[str] = []
        self._timer: asyncio.TimerHandle | None = None
        self._warned_stray = False

    @property
    def receiving(self) -> bool:
        return self._header is not None

    def handle(self, packet):
        """Feed every packet here; anything that isn't a camera packet is ignored."""
        match packet:
            case CameraHeader():
                self._on_header(packet)
            case CameraChunk():
                self._on_chunk(packet)

    # ---- internals ---------------------------------------------------------

    def _on_header(self, header: CameraHeader):
        if self.receiving:
            self._log(f"{NAME} new header while receiving: dropped the previous image "
                      f"({len(self._chunks)}/{self._header.nchunks} chunks)")
            self._reset()
        self._header = header
        self._log(f"{NAME} header: expecting {header.nchunks} chunks, "
                  f"timeout {header.timeout_ms} ms between chunks")
        if header.nchunks == 0:
            self._log(f"{NAME} header says 0 chunks, nothing to receive")
            self._reset()
            return
        self._restart_timer()

    def _on_chunk(self, chunk: CameraChunk):
        if not self.receiving:
            if not self._warned_stray:      # once, not per chunk
                self._warned_stray = True
                self._log(f"{NAME} chunk {chunk.index} arrived without a header, ignored "
                          f"(repeats hidden)")
            return
        n = self._header.nchunks
        if chunk.index >= n:
            self._problems.append(f"chunk {chunk.index} out of range (0..{n - 1}), ignored")
        elif chunk.index in self._chunks:
            self._problems.append(f"chunk {chunk.index} received twice, kept the first")
        else:
            if len(chunk.data) != chunk.length:
                self._problems.append(f"chunk {chunk.index}: len field {chunk.length}, "
                                      f"but {len(chunk.data)} data bytes")
            self._chunks[chunk.index] = chunk.data
        if len(self._chunks) == n:
            self._finish()
        else:
            self._restart_timer()

    def _restart_timer(self):
        if self._timer is not None:
            self._timer.cancel()
        delay = self._header.timeout_ms / 1000
        self._timer = asyncio.get_running_loop().call_later(delay, self._on_timeout)

    def _on_timeout(self):
        self._timer = None
        n = self._header.nchunks
        missing = [i for i in range(n) if i not in self._chunks]
        shown = ", ".join(map(str, missing[:10])) + (" ..." if len(missing) > 10 else "")
        self._log(f"{NAME} timeout ({self._header.timeout_ms} ms without a chunk): "
                  f"got {len(self._chunks)}/{n} chunks, missing {shown}. Image dropped")
        self._report_problems()
        self._reset()

    def _finish(self):
        image = b"".join(self._chunks[i] for i in range(self._header.nchunks))
        if not image.startswith(b"\xff\xd8"):
            self._problems.append("data does not start with the JPEG SOI marker (FF D8)")
        if not image.rstrip(b"\x00").endswith(b"\xff\xd9"):
            self._problems.append("data does not end with the JPEG EOI marker (FF D9)")
        self._dir.mkdir(parents=True, exist_ok=True)
        path = self._dir / f"photo_{datetime.now():%Y%m%d_%H%M%S_%f}.jpg"
        path.write_bytes(image)
        self._log(f"{NAME} image complete: {self._header.nchunks} chunks, "
                  f"{len(image)} bytes, saved to {path}")
        self._report_problems()
        self._reset()

    def _report_problems(self):
        for p in self._problems[:10]:
            self._log(f"{NAME}   warning: {p}")
        if len(self._problems) > 10:
            self._log(f"{NAME}   ... and {len(self._problems) - 10} more warnings")

    def _reset(self):
        if self._timer is not None:
            self._timer.cancel()
        self._timer = None
        self._header = None
        self._chunks = {}
        self._problems = []
