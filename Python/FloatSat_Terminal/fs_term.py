import asyncio
import os
import shutil
import signal
import struct
import sys
import threading
from enum import IntEnum
import time

import websockets

try:
    import readline  # line editing + history for input()
except ImportError:  # Windows without pyreadline3: input() still works, just no history
    readline = None

try:
    import termios   # used to put the terminal back exactly as we found it
except ImportError:
    termios = None


PROMPT = "fs_term$ "

TELEMETRY_ROW = 1
GAP_ROWS = 3
PROMPT_ROW = TELEMETRY_ROW + GAP_ROWS + 1   # row 5: fixed prompt line
LOG_TOP = PROMPT_ROW + 1                    # rows 6..bottom: scrolling log

PROMPT_HOME = f"\x1b[{PROMPT_ROW};1H\x1b[2K"   # jump to the prompt row and clear it

CMD_MAGIC = 0xCAFE


class PacketType(IntEnum):
    FAST = 0
    SLOW = 1


# '<' = little-endian, no padding (matches the STM32 side)
PACKET_FORMATS = {
    PacketType.FAST: struct.Struct("<BQ3ff"),
    # PacketType.SLOW: struct.Struct("<B..."),
}


# --------------------------------------------------------------------------
# Console layout
#   row 1        telemetry (overwritten in place)
#   rows 2-4     empty
#   row 5        prompt (cleared after every Enter)
#   rows 6..end  scrolling log (info messages + echo of sent commands)
# --------------------------------------------------------------------------

class Console:
    def __init__(self):
        self.out = sys.stdout
        self._lock = threading.Lock()
        self._tty_state = None
        self._update_size()
        self.log_row = LOG_TOP      # where the next log line goes

    def _update_size(self):
        size = shutil.get_terminal_size()
        self.rows, self.cols = size.lines, size.columns

    def _write(self, s: str):
        # One write + flush per update, so updates don't interleave with each
        # other or with the characters readline is echoing on the prompt row.
        with self._lock:
            self.out.write(s)
            self.out.flush()

    def start(self):
        if termios and sys.stdin.isatty():
            self._tty_state = termios.tcgetattr(sys.stdin)
        if os.name == "nt":
            os.system("")  # enables ANSI escape processing in the Windows console
        # clear screen, restrict scrolling to the log area, park cursor on the prompt row
        self._write(f"\x1b[2J\x1b[{LOG_TOP};{self.rows}r\x1b[{PROMPT_ROW};1H")

    def stop(self):
        # full-screen scrolling again, cursor after the last log line
        self._write(f"\x1b[r\x1b[{self.log_row};1H")
        if self._tty_state is not None:
            termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self._tty_state)

    def resize(self):
        self._update_size()
        self.log_row = min(self.log_row, self.rows)
        # setting the region homes the cursor, so save/restore around it
        self._write(f"\x1b7\x1b[{LOG_TOP};{self.rows}r\x1b8")

    def home_prompt(self):
        self._write(PROMPT_HOME)

    def telemetry(self, text: str):
        # save cursor, jump to the telemetry row, clear it, write, restore cursor
        self._write(f"\x1b7\x1b[{TELEMETRY_ROW};1H\x1b[2K{text[:self.cols]}\x1b8")

    def info(self, text: str):
        lines = text.replace("\r\n", "\n").split("\n")
        # screen rows this message takes, counting lines that wrap
        used = sum(max(1, -(-len(line) // self.cols)) for line in lines)
        body = "\r\n".join(lines)
        # Write at the log's next free row; once that is the bottom row, the
        # trailing \r\n scrolls the log region (and only the log region) up.
        self._write(f"\x1b7\x1b[{self.log_row};1H{body}\r\n\x1b8")
        self.log_row = min(self.log_row + used, self.rows)


# --------------------------------------------------------------------------
# Packets
# --------------------------------------------------------------------------

def parse_packet(data: bytes):
    if not data:
        return None
    fmt = PACKET_FORMATS.get(data[0])
    if fmt is None or len(data) != fmt.size:
        return None
    return fmt.unpack(data)


def print_packet(console: Console, packet):
    match packet[0]:
        case PacketType.FAST:
            _, t, x, y, z, rw = packet
            console.telemetry(f"({t}) Orientation: {{{x:.2f} {y:.2f} {z:.2f}}} RW: {rw:.2f}")


def hexdump(data: bytes) -> str:
    return ' '.join(f'{b:02x}' for b in data)


# --------------------------------------------------------------------------
# Listener (websocket -> screen) and writer (keyboard -> websocket)
# --------------------------------------------------------------------------

async def listen(ws, console: Console):
    try:
        async for message in ws:
            if isinstance(message, str):        # text frame -> info
                console.info(message.rstrip("\r\n"))
            else:                               # binary frame -> telemetry
                packet = parse_packet(message)
                if packet:
                    print_packet(console, packet)
    except websockets.exceptions.ConnectionClosed:
        pass
    console.info("Disconnected from the server")


def start_input_thread(console: Console, loop, queue: asyncio.Queue):
    # input() blocks, so it lives in its own thread and hands lines to the loop.
    # Daemon thread so it never keeps the program alive on exit.
    # After Enter the cursor drops to row 6, so every prompt starts by jumping
    # back to row 5 and clearing it. With readline the jump goes inside the
    # prompt string, so it is always emitted by readline itself, in order, right
    # before the prompt text. \001 ... \002 tell readline those bytes take no
    # space on screen, so its cursor math for line editing stays correct.
    if readline:
        prompt = f"\001{PROMPT_HOME}\002{PROMPT}"
    else:
        prompt = PROMPT

    def reader():
        while True:
            if readline is None:
                console.home_prompt()
            try:
                line = input(prompt)
            except EOFError:        # Ctrl+D
                line = None
            try:
                loop.call_soon_threadsafe(queue.put_nowait, line)
            except RuntimeError:    # loop already closed
                return
            if line is None:
                return

    threading.Thread(target=reader, daemon=True).start()


async def handle_console(ws, console: Console, queue: asyncio.Queue):
    try:
        while True:
            line = await queue.get()
            if line is None:            # Ctrl+D
                return
            if not line.strip():        # plain Enter: just clears the prompt
                continue
            console.info(f"{PROMPT}{line}")   # keep a record of what was sent
            payload = line.encode()
            out = struct.pack(f"<HHH{len(payload)}s", CMD_MAGIC, len(payload), 0x00, payload)
            await ws.send(out[0:6])          # Send header first
            time.sleep(0.001)
            await ws.send(out[6:])          # send body
    except websockets.exceptions.ConnectionClosed:
        pass


# --------------------------------------------------------------------------

async def main(ip: str) -> int:
    uri = f"ws://{ip}/ws"
    try:
        ws = await websockets.connect(uri, open_timeout=15)
    except (OSError, TimeoutError, asyncio.TimeoutError,
            websockets.exceptions.WebSocketException) as e:
        # OSError: refused / unreachable / bad hostname
        # TimeoutError: nothing answered within open_timeout
        # WebSocketException: something answered, but not a websocket at /ws (or bad URI)
        print(f"Could not open a WebSocket at {uri}: {type(e).__name__}: {e}")
        return 1

    console = Console()
    console.start()
    try:
        console.info(f"Connected to {uri}  (Ctrl+D or Ctrl+C to quit)")

        loop = asyncio.get_running_loop()
        if hasattr(signal, "SIGWINCH"):
            loop.add_signal_handler(signal.SIGWINCH, console.resize)

        queue = asyncio.Queue()
        start_input_thread(console, loop, queue)

        tasks = {
            asyncio.create_task(listen(ws, console)),
            asyncio.create_task(handle_console(ws, console, queue)),
        }
        # whichever ends first (server closed, or user hit Ctrl+D) stops the other
        done, pending = await asyncio.wait(tasks, return_when=asyncio.FIRST_COMPLETED)
        for t in pending:
            t.cancel()
        await asyncio.gather(*pending, return_exceptions=True)
        for t in done:
            t.result()   # re-raise anything unexpected
    finally:
        console.stop()
        await ws.close()
    return 0


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} <ip[:port]>")
        sys.exit(2)
    try:
        sys.exit(asyncio.run(main(sys.argv[1])))
    except KeyboardInterrupt:
        pass