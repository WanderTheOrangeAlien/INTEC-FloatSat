"""`test angle`: passively record the control angle reported by the FloatSat.

Sends nothing to the FloatSat. For DURATION_S seconds (measured on this
computer's clock) it collects every control debug packet and writes one CSV
row per packet.

Each row is (timestamp, angle). timestamp is the FloatSat's own timestamp from
the packet, exactly as sent (no unit conversion), and angle is
control_data_t.angle.

The file is closed on every exit path, so an aborted run keeps what it
recorded. Nothing needs to be undone at the end, since no command was sent.
"""
import asyncio
import csv
from datetime import datetime
from pathlib import Path

from ..protocol import ControlDebug

DURATION_S = 40.0               # how long to record, in seconds
RESULTS_DIR = Path("results")

NAME = "[test angle]"


async def run(link, log):
    RESULTS_DIR.mkdir(exist_ok=True)
    path = RESULTS_DIR / f"angle_log_{datetime.now():%Y%m%d_%H%M%S}.csv"
    log(f"{NAME} recording for {DURATION_S:g} s -> {path}  ('abort' to stop)")

    loop = asyncio.get_running_loop()
    rows = 0

    with path.open("w", newline="") as f, link.subscribe(ControlDebug) as control:
        writer = csv.writer(f)
        writer.writerow(["timestamp", "w","angle"])
        deadline = loop.time() + DURATION_S
        try:
            while (remaining := deadline - loop.time()) > 0:
                try:
                    received_at, packet = await asyncio.wait_for(control.get(), remaining)
                except asyncio.TimeoutError:
                    break
                if received_at > deadline:      # arrived after the recording window
                    break
                writer.writerow([packet.timestamp, round(packet.w,6) ,round(packet.angle, 6)])
                rows += 1
        except asyncio.CancelledError:
            log(f"{NAME} aborted, {rows} rows saved to {path}")
            raise

    if rows == 0:
        log(f"{NAME} no control debug packets received. Is the debug output enabled?")
    else:
        log(f"{NAME} done, {rows} rows saved to {path}")
