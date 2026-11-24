"""Wires everything together: connection, input routing and test procedures."""
import asyncio
import signal

import websockets
import websockets.exceptions

from .camera import CameraReceiver
from .console import PROMPT, Console
from .link import Link
from .procedures import PROCEDURES
from . import scripts
from .view import STATUS_ROWS, show_packet


# Reconnection. The library pings the ESP32 every PING_INTERVAL_S; if no pong
# comes back within PING_TIMEOUT_S the link counts as lost, after waiting up to
# CLOSE_TIMEOUT_S for a closing handshake. That catches an ESP32 that reset or
# left WiFi without closing the socket, in about 5 + 5 + 2 = 12 s at worst.
RETRY_DELAY_S = 2.0
OPEN_TIMEOUT_S = 5.0
PING_INTERVAL_S = 5.0
PING_TIMEOUT_S = 5.0
CLOSE_TIMEOUT_S = 2.0

_CONNECT_ERRORS = (OSError, TimeoutError, asyncio.TimeoutError,
                   websockets.exceptions.WebSocketException)


class App:
    def __init__(self, uri: str, console: Console):
        self.uri = uri
        self.console = console
        self.camera = CameraReceiver(console.info)
        self.link: Link | None = None       # None while disconnected
        self.procedure: asyncio.Task | None = None

    def _on_packet(self, packet):
        show_packet(self.console, packet)
        self.camera.handle(packet)

    @property
    def procedure_running(self) -> bool:
        return self.procedure is not None and not self.procedure.done()

    # ---- commands typed at the prompt -------------------------------------

    async def handle_line(self, line: str):
        words = line.split()
        if not words:                       # plain Enter: just clears the prompt
            return
        self.console.info(f"{PROMPT}{line}")      # keep a record of what was typed

        match words:
            case ["test", name] if name in PROCEDURES and self.link is None:
                self.console.info("not connected, test not started")
            case ["test", name] if name in PROCEDURES:
                self.start_procedure(name)
            case ["test", *_]:
                self.console.info(f"usage: test <{'|'.join(PROCEDURES)}>")
            case ["script", *args]:
                self.start_script(args)
            case ["abort"]:
                if self.procedure_running:
                    self.procedure.cancel()
                else:
                    self.console.info("no test or script is running")
            case _ if self.procedure_running:
                # keep the test's measurements clean
                self.console.info("a test or script is running, type 'abort' to stop it first")
            case _ if self.link is None:
                self.console.info("not connected, command not sent")
            case _:
                try:
                    await self.link.send_command(line)
                except websockets.exceptions.ConnectionClosed:
                    self.console.info("connection lost, command not sent")

    # ---- procedures -------------------------------------------------------

    def start_procedure(self, name: str):
        self._start_task(f"test {name}", PROCEDURES[name](self.link, self.console.info))

    def start_script(self, args: list[str]):
        try:
            sequences = scripts.load_sequences()
        except scripts.SequenceError as e:
            self.console.info(f"[script] {e}")
            return
        if len(args) != 1 or args[0] not in sequences:
            if args:
                self.console.info(f"[script] unknown sequence {' '.join(args)!r}")
            names = ", ".join(sequences) or f"none (add them to {scripts.SEQUENCES_FILE.name})"
            self.console.info(f"usage: script <name>   sequences: {names}")
            return
        if self.link is None:
            self.console.info("not connected, script not started")
            return
        name = args[0]
        self._start_task(f"script {name}",
                         scripts.run(name, sequences[name], self.link, self.console.info))

    def _start_task(self, label: str, coro):
        """Run a test or script in the single procedure slot, so 'abort' and
        a dropped link cancel it the same way."""
        if self.procedure_running:
            coro.close()                    # never started, avoid the "never awaited" warning
            self.console.info("a test or script is already running")
            return
        self.procedure = asyncio.create_task(self._run_procedure(label, coro))

    async def _run_procedure(self, label: str, coro):
        try:
            # a test is bound to the link it started on; if that link drops,
            # the test is cancelled (see _connection_loop)
            await coro
        except asyncio.CancelledError:
            pass                            # aborted; the procedure already said so
        except Exception as e:
            self.console.info(f"[{label}] failed: {type(e).__name__}: {e}")

    async def stop_procedure(self):
        """Cancel a running procedure and wait for its cleanup (e.g. motor off)."""
        if self.procedure_running:
            self.procedure.cancel()
            await asyncio.gather(self.procedure, return_exceptions=True)

    # ---- connection -------------------------------------------------------

    async def _connect(self):
        """Try until a connection opens. Reports the first failure only."""
        failures = 0
        while True:
            try:
                return await websockets.connect(self.uri,
                                                open_timeout=OPEN_TIMEOUT_S,
                                                ping_interval=PING_INTERVAL_S,
                                                ping_timeout=PING_TIMEOUT_S,
                                                close_timeout=CLOSE_TIMEOUT_S)
            except _CONNECT_ERRORS as e:
                # OSError: refused / unreachable / bad hostname
                # TimeoutError: nothing answered within OPEN_TIMEOUT_S
                # WebSocketException: something answered, but not a websocket at /ws
                if failures == 0:
                    self.console.info(f"Could not connect to {self.uri}: {type(e).__name__}: {e}. "
                                      f"Retrying every {RETRY_DELAY_S:g} s...")
                failures += 1
                await asyncio.sleep(RETRY_DELAY_S)

    async def _connection_loop(self):
        """Connect, receive until the link drops, repeat. Runs until cancelled."""
        while True:
            ws = await self._connect()
            self.console.info(f"Connected to {self.uri}")
            self.link = Link(ws,
                             on_packet=self._on_packet,
                             on_text=self.console.info,
                             on_warning=lambda msg: self.console.info(f"[warn] {msg}"))
            try:
                await self.link.run()
            finally:
                self.link = None
                await ws.close()
            if self.procedure_running:
                self.console.info("connection lost, aborting the running test or script")
                await self.stop_procedure()
            self.console.info(f"Disconnected. Reconnecting to {self.uri}...")

    # ---- main loop ----------------------------------------------------------

    async def _command_loop(self, queue: asyncio.Queue):
        while (line := await queue.get()) is not None:     # None = Ctrl+D
            await self.handle_line(line)

    async def run(self):
        queue = asyncio.Queue()
        self.console.start_input(asyncio.get_running_loop(), queue)
        tasks = {
            asyncio.create_task(self._connection_loop()),
            asyncio.create_task(self._command_loop(queue)),
        }
        try:
            # the connection loop never ends on its own, so this waits for Ctrl+D
            # (or an unexpected error in either task)
            done, _ = await asyncio.wait(tasks, return_when=asyncio.FIRST_COMPLETED)
        finally:
            # also runs on Ctrl+C: a running test still gets to stop the motor
            await self.stop_procedure()
            for t in tasks:
                t.cancel()
            await asyncio.gather(*tasks, return_exceptions=True)
        for t in done:
            t.result()      # re-raise anything unexpected


async def main(ip: str) -> int:
    uri = f"ws://{ip}/ws"
    console = Console(STATUS_ROWS)
    console.start()
    try:
        console.info(f"Connecting to {uri}  (Ctrl+D or Ctrl+C to quit)")
        loop = asyncio.get_running_loop()
        if hasattr(signal, "SIGWINCH"):
            loop.add_signal_handler(signal.SIGWINCH, console.resize)

        await App(uri, console).run()
    finally:
        console.stop()
    return 0
