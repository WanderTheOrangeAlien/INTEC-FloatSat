"""`test mag`: record the x and y components of the magnetometer.

  1. send `control --disable` and `sys --debug-id 1` (IMU debug packets)
  2. for DURATION_S seconds (this computer's clock), write one CSV row per
     IMU debug packet: timestamp, mag_x, mag_y
     (timestamp is the FloatSat's own, exactly as sent)

No commands are sent during the recording or at the end. The file is closed
on every exit path, so an aborted run keeps what it recorded.
"""
import asyncio
import csv
from datetime import datetime
from pathlib import Path

from ..protocol import DebugId, ImuDebug

DURATION_S = 40.0               # how long to record, in seconds
RESULTS_DIR = Path("results")
START_COMMANDS = ["control --enable", f"sys --debug-id {int(DebugId.IMU)}"]

NAME = "[test mag]"


async def run(link, log):
    RESULTS_DIR.mkdir(exist_ok=True)
    path = RESULTS_DIR / f"mag_log_{datetime.now():%Y%m%d_%H%M%S}.csv"
    log(f"{NAME} recording for {DURATION_S:g} s -> {path}  ('abort' to stop)")

    loop = asyncio.get_running_loop()
    rows = 0

    # Subscribe before switching the debug output, so the first IMU packets are kept.
    with path.open("w", newline="") as f, link.subscribe(ImuDebug) as imu:
        writer = csv.writer(f)
        writer.writerow(["timestamp", "mag_x", "mag_y"])
        for command in START_COMMANDS:
            await link.send_command(command)
            log(f"{NAME} sent '{command}'")

        deadline = loop.time() + DURATION_S
        try:
            while (remaining := deadline - loop.time()) > 0:
                try:
                    received_at, packet = await asyncio.wait_for(imu.get(), remaining)
                except asyncio.TimeoutError:
                    break
                if received_at > deadline:      # arrived after the recording window
                    break
                writer.writerow([packet.timestamp,
                                 round(packet.mag.x, 6), round(packet.mag.y, 6)])
                rows += 1
        except asyncio.CancelledError:
            log(f"{NAME} aborted, {rows} rows saved to {path}")
            raise

    if rows == 0:
        log(f"{NAME} no IMU debug packets received. Is the debug output enabled?")
    else:
        log(f"{NAME} done, {rows} rows saved to {path}")
