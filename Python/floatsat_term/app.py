"""Wires everything together: connection, input routing and test procedures."""
import asyncio
import signal

import websockets

from .console import PROMPT, Console
from .link import Link
from .procedures import PROCEDURES
from .view import STATUS_ROWS, show_packet


class App:
    def __init__(self, ws, console: Console):
        self.console = console
        self.link = Link(ws,
                         on_packet=lambda packet: show_packet(console, packet),
                         on_text=console.info,
                         on_warning=lambda msg: console.info(f"[warn] {msg}"))
        self.procedure: asyncio.Task | None = None

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
            case ["test", name] if name in PROCEDURES:
                self.start_procedure(name)
            case ["test", *_]:
                self.console.info(f"usage: test <{'|'.join(PROCEDURES)}>")
            case ["abort"]:
                if self.procedure_running:
                    self.procedure.cancel()
                else:
                    self.console.info("no test is running")
            case _ if self.procedure_running:
                # keep the test's measurements clean
                self.console.info("a test is running, type 'abort' to stop it first")
            case _:
                await self.link.send_command(line)

    # ---- procedures -------------------------------------------------------

    def start_procedure(self, name: str):
        if self.procedure_running:
            self.console.info("a test is already running")
            return
        self.procedure = asyncio.create_task(self._run_procedure(name))

    async def _run_procedure(self, name: str):
        try:
            await PROCEDURES[name](self.link, self.console.info)
        except asyncio.CancelledError:
            pass                            # aborted; the procedure already said so
        except Exception as e:
            self.console.info(f"[test {name}] failed: {type(e).__name__}: {e}")

    async def stop_procedure(self):
        """Cancel a running procedure and wait for its cleanup (e.g. motor off)."""
        if self.procedure_running:
            self.procedure.cancel()
            await asyncio.gather(self.procedure, return_exceptions=True)

    # ---- main loop ----------------------------------------------------------

    async def _command_loop(self, queue: asyncio.Queue):
        try:
            while (line := await queue.get()) is not None:     # None = Ctrl+D
                await self.handle_line(line)
        except websockets.exceptions.ConnectionClosed:
            pass

    async def run(self):
        queue = asyncio.Queue()
        self.console.start_input(asyncio.get_running_loop(), queue)
        tasks = {
            asyncio.create_task(self.link.run()),
            asyncio.create_task(self._command_loop(queue)),
        }
        try:
            # whichever ends first (server closed, or Ctrl+D) ends the session
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
    try:
        ws = await websockets.connect(uri, open_timeout=5)
    except (OSError, TimeoutError, asyncio.TimeoutError,
            websockets.exceptions.WebSocketException) as e:
        # OSError: refused / unreachable / bad hostname
        # TimeoutError: nothing answered within open_timeout
        # WebSocketException: something answered, but not a websocket at /ws (or bad URI)
        print(f"Could not open a WebSocket at {uri}: {type(e).__name__}: {e}")
        return 1

    console = Console(STATUS_ROWS)
    console.start()
    try:
        console.info(f"Connected to {uri}  (Ctrl+D or Ctrl+C to quit)")
        loop = asyncio.get_running_loop()
        if hasattr(signal, "SIGWINCH"):
            loop.add_signal_handler(signal.SIGWINCH, console.resize)

        await App(ws, console).run()
        console.info("Disconnected")
    finally:
        console.stop()
        await ws.close()
    return 0
