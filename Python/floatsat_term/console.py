"""Terminal layout and keyboard input. Knows nothing about packets or the link.

    rows 1..status_rows   fixed rows, overwritten in place with status()
    next row              prompt (cleared after every Enter)
    remaining rows        scrolling log

Which packet goes on which fixed row is decided in view.py, which also says
how many fixed rows it needs (STATUS_ROWS).
"""
import asyncio
import os
import shutil
import sys
import threading

try:
    import readline  # line editing + history for input()
except ImportError:  # Windows without pyreadline3: input() still works, just no history
    readline = None

try:
    import termios   # used to put the terminal back exactly as we found it
except ImportError:
    termios = None


PROMPT = "fs_term$ "


class Console:
    def __init__(self, status_rows: int, gap_rows: int = 0):
        """`gap_rows` is the number of blank rows between the fixed rows and the prompt."""
        self.out = sys.stdout
        self._lock = threading.Lock()
        self._tty_state = None
        self._update_size()

        self.prompt_row = status_rows + gap_rows + 1
        self.log_top = self.prompt_row + 1
        self.log_row = self.log_top     # where the next log line goes
        # jump to the prompt row and clear it
        self._prompt_home = f"\x1b[{self.prompt_row};1H\x1b[2K"

    def _update_size(self):
        size = shutil.get_terminal_size()
        self.rows, self.cols = size.lines, size.columns

    def _write(self, s: str):
        # One write + flush per update, so updates don't interleave with each
        # other or with the characters readline is echoing on the prompt row.
        with self._lock:
            self.out.write(s)
            self.out.flush()

    # ---- lifecycle -------------------------------------------------------

    def start(self):
        if termios and sys.stdin.isatty():
            self._tty_state = termios.tcgetattr(sys.stdin)
        if os.name == "nt":
            os.system("")  # enables ANSI escape processing in the Windows console
        # clear screen, restrict scrolling to the log area, park cursor on the prompt row
        self._write(f"\x1b[2J\x1b[{self.log_top};{self.rows}r\x1b[{self.prompt_row};1H")

    def stop(self):
        # full-screen scrolling again, cursor after the last log line
        self._write(f"\x1b[r\x1b[{self.log_row};1H")
        if self._tty_state is not None:
            termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self._tty_state)

    def resize(self):
        self._update_size()
        self.log_row = min(self.log_row, self.rows)
        # setting the region homes the cursor, so save/restore around it
        self._write(f"\x1b7\x1b[{self.log_top};{self.rows}r\x1b8")

    # ---- output ----------------------------------------------------------

    def status(self, row: int, *lines: str):
        """Overwrite consecutive fixed rows starting at `row`, in a single write."""
        body = "".join(f"\x1b[{row + i};1H\x1b[2K{line[:self.cols]}"
                       for i, line in enumerate(lines))
        self._write(f"\x1b7{body}\x1b8")

    def info(self, text: str):
        """Append a message to the scrolling log."""
        lines = text.replace("\r\n", "\n").split("\n")
        # screen rows this message takes, counting lines that wrap
        used = sum(max(1, -(-len(line) // self.cols)) for line in lines)
        body = "\r\n".join(lines)
        # Write at the log's next free row; once that is the bottom row, the
        # trailing \r\n scrolls the log region (and only the log region) up.
        self._write(f"\x1b7\x1b[{self.log_row};1H{body}\r\n\x1b8")
        self.log_row = min(self.log_row + used, self.rows)

    # ---- input -----------------------------------------------------------

    def start_input(self, loop: asyncio.AbstractEventLoop, queue: asyncio.Queue):
        """Read lines in a background thread and put them on `queue` (None on Ctrl+D)."""
        # After Enter the cursor drops to the row below the prompt, so every prompt
        # starts by jumping back to the prompt row and clearing it. With readline the
        # jump goes inside the prompt string, so it is always emitted by readline
        # itself, in order, right before the prompt text. \001 ... \002 tell readline
        # those bytes take no space on screen, so its cursor math for line editing
        # stays correct.
        prompt = f"\001{self._prompt_home}\002{PROMPT}" if readline else PROMPT

        def reader():
            while True:
                if readline is None:
                    self._write(self._prompt_home)
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

        # daemon thread, so a blocked input() never keeps the program alive on exit
        threading.Thread(target=reader, daemon=True).start()
