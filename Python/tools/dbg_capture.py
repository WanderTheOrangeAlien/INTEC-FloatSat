#!/usr/bin/env python3
"""
dbg_capture.py - capture a debug buffer sent over a serial port.

Stream format (everything after the start marker):

    ...ASCII text... 0x03 LEN_0 LEN_1 <LEN raw bytes> ...ASCII text... 0x03 ...

  * Capture begins right after the marker "DBG_BUFF_START" is seen.
  * Bytes are treated as ASCII until a 0x03 byte arrives.
  * The next two bytes give the length n of a binary block (little-endian
    by default, see --endian).
  * The next n bytes are written to the output as hex ("0x1F 0xA0 ...").
  * After those n bytes, parsing goes back to ASCII.
  * The capture ends when no byte arrives for --timeout seconds (default 1 s),
    and the result is saved as a .txt file.

Usage:
    python dbg_capture.py COM5 115200
    python dbg_capture.py /dev/ttyACM0 921600 -o dump.txt --endian big
"""

import argparse
import sys
import time
from datetime import datetime

try:
    import serial
except ImportError:
    sys.exit("pyserial is required:  pip install pyserial")

ETX = 0x03


class DebugStreamParser:
    """Byte-by-byte state machine. Safe to feed arbitrary chunk sizes."""

    SEARCH, ASCII, LENGTH, HEX = range(4)

    def __init__(self, marker: bytes, endian: str = "little",
                 per_line: int = 16, echo: bool = True):
        self.marker = marker
        self.endian = endian
        self.per_line = per_line
        self.echo = echo

        self.state = self.SEARCH
        self.window = bytearray()     # rolling window used to find the marker
        self.ascii_buf = bytearray()  # pending ASCII bytes not yet emitted
        self.len_buf = bytearray()    # the two length bytes
        self.hex_total = 0
        self.hex_done = 0
        self.blocks = 0

        self.out = []                 # list of formatted text pieces
        self._last_char = "\n"

    @property
    def started(self) -> bool:
        return self.state != self.SEARCH

    # ------------------------------------------------------------------ output
    def _emit(self, text: str) -> None:
        if not text:
            return
        self.out.append(text)
        self._last_char = text[-1]
        if self.echo:
            sys.stdout.write(text)
            sys.stdout.flush()

    def _flush_ascii(self) -> None:
        if self.ascii_buf:
            # Non-ASCII bytes are kept visible as \xNN instead of being dropped
            self._emit(self.ascii_buf.decode("ascii", errors="backslashreplace"))
            self.ascii_buf.clear()

    # ----------------------------------------------------------------- parsing
    def feed(self, data: bytes) -> None:
        for b in data:
            self._feed_byte(b)
        self._flush_ascii()           # keeps the console echo live

    def _feed_byte(self, b: int) -> None:
        if self.state == self.SEARCH:
            self.window.append(b)
            if len(self.window) > len(self.marker):
                del self.window[0]
            if self.window == self.marker:
                self.state = self.ASCII
                if self.echo:
                    print(f"\n[marker found, capturing]\n", file=sys.stderr)

        elif self.state == self.ASCII:
            if b == ETX:
                self._flush_ascii()
                self.len_buf.clear()
                self.state = self.LENGTH
            else:
                self.ascii_buf.append(b)

        elif self.state == self.LENGTH:
            self.len_buf.append(b)
            if len(self.len_buf) == 2:
                self.hex_total = int.from_bytes(self.len_buf, self.endian)
                self.hex_done = 0
                self.blocks += 1
                if self.hex_total == 0:
                    self.state = self.ASCII
                else:
                    if self._last_char != "\n":
                        self._emit("\n")   # hex block starts on its own line
                    self.state = self.HEX

        elif self.state == self.HEX:
            if self.hex_done > 0:
                sep = "\n" if self.hex_done % self.per_line == 0 else " "
            else:
                sep = ""
            self._emit(f"{sep}0x{b:02X}")
            self.hex_done += 1
            if self.hex_done == self.hex_total:
                self._emit("\n")
                self.state = self.ASCII

    def finish(self) -> str:
        """Flush pending data and return the full captured text."""
        self._flush_ascii()
        if self.state == self.LENGTH:
            self._emit(f"\n[stream ended while reading length bytes "
                       f"({len(self.len_buf)}/2 received)]\n")
        elif self.state == self.HEX:
            self._emit(f"\n[stream ended inside hex block: "
                       f"{self.hex_done}/{self.hex_total} bytes received]\n")
        return "".join(self.out)


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Capture a serial debug buffer starting at a marker, "
                    "converting 0x03-framed binary blocks to hex text.")
    ap.add_argument("port", help="serial port, e.g. COM5 or /dev/ttyACM0")
    ap.add_argument("baudrate", nargs="?", type=int, default=115200,
                    help="baud rate (default: 115200)")
    ap.add_argument("-o", "--output",
                    help="output .txt file (default: dbg_dump_<timestamp>.txt)")
    ap.add_argument("-t", "--timeout", type=float, default=1.0,
                    help="seconds of silence that end the capture (default: 1.0)")
    ap.add_argument("--endian", choices=("little", "big"), default="little",
                    help="byte order of the 2-byte length (default: little)")
    ap.add_argument("--per-line", type=int, default=16,
                    help="hex values per line (default: 16)")
    ap.add_argument("--marker", default="DBG_BUFF_START",
                    help="start marker (default: DBG_BUFF_START)")
    ap.add_argument("-q", "--quiet", action="store_true",
                    help="don't echo the capture to the console")
    args = ap.parse_args()

    output = args.output or datetime.now().strftime("results/dbg_buffer/dbg_dump_%Y%m%d_%H%M%S.txt")
    parser = DebugStreamParser(args.marker.encode("ascii"), args.endian,
                               max(1, args.per_line), echo=not args.quiet)

    try:
        # serial_for_url accepts normal port names as well as pyserial URLs
        ser = serial.serial_for_url(args.port, baudrate=args.baudrate,
                                    timeout=0.05)
    except serial.SerialException as e:
        print(f"Could not open {args.port}: {e}", file=sys.stderr)
        return 1

    print(f"Listening on {args.port} @ {args.baudrate} baud, "
          f"waiting for '{args.marker}' (Ctrl+C to abort)...", file=sys.stderr)

    last_rx = 0.0
    try:
        with ser:
            while True:
                data = ser.read(ser.in_waiting or 1)
                now = time.monotonic()
                if data:
                    parser.feed(data)
                    if parser.started:
                        last_rx = now
                elif parser.started and now - last_rx >= args.timeout:
                    break
    except KeyboardInterrupt:
        print("\n[interrupted]", file=sys.stderr)
    except serial.SerialException as e:
        print(f"\n[serial error: {e}]", file=sys.stderr)

    if not parser.started:
        print("Marker never received, nothing saved.", file=sys.stderr)
        return 2

    text = parser.finish()
    with open(output, "w", encoding="utf-8", newline="") as f:
        f.write(text)

    print(f"\nSaved {len(text)} chars ({parser.blocks} hex block(s)) "
          f"to {output}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
