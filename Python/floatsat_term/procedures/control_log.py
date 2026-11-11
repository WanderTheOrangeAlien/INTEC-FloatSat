"""`test control`: run the control loop and record every control debug packet.

  1. send `control --enable`
  2. for DURATION_S seconds (this computer's clock), write one CSV row per
     control debug packet, with every field of the packet:
     timestamp, angle, angle_ref, w, w_ref, pwm_out, rw_speed
     (timestamp is the FloatSat's own, exactly as sent)
  3. close the file, then send `control --disable` and `motor --pwm 0.00`

Step 3 also runs when the test is aborted, fails, or the console exits
mid-test, so the file always keeps what was recorded and the wheel is always
stopped. The control loop is disabled first, so it can't command the motor
again after the PWM is set to 0.
"""
import asyncio
import csv
import dataclasses
from datetime import datetime
from pathlib import Path

from ..protocol import ControlDebug

DURATION_S = 40.0               # how long to record, in seconds
RESULTS_DIR = Path("results")
START_COMMANDS = ["control --debug-id 1", "control --enable"]
STOP_COMMANDS = ["control --disable", "motor --pwm 0.00"]   # sent in this order

NAME = "[test control]"
FIELDS = [f.name for f in dataclasses.fields(ControlDebug)]


async def _send_stop_commands(link, log):
    # each one is tried even if the one before it failed
    for command in STOP_COMMANDS:
        try:
            await link.send_command(command)
            log(f"{NAME} sent '{command}'")
        except Exception as e:
            log(f"{NAME} could not send '{command}' ({type(e).__name__}). "
                f"STOP THE MOTOR MANUALLY")


async def run(link, log):
    RESULTS_DIR.mkdir(exist_ok=True)
    path = RESULTS_DIR / f"control_log_{datetime.now():%Y%m%d_%H%M%S}.csv"
    log(f"{NAME} recording for {DURATION_S:g} s -> {path}  ('abort' to stop)")

    loop = asyncio.get_running_loop()
    rows = 0
    aborted = False

    try:
        # Subscribe before enabling, so the first packets after enable are kept.
        with path.open("w", newline="") as f, link.subscribe(ControlDebug) as control:
            writer = csv.writer(f)
            writer.writerow(FIELDS)
            for command in START_COMMANDS:
                await link.send_command(command)
                log(f"{NAME} sent '{command}'")

            deadline = loop.time() + DURATION_S
            try:
                while (remaining := deadline - loop.time()) > 0:
                    try:
                        received_at, packet = await asyncio.wait_for(control.get(), remaining)
                    except asyncio.TimeoutError:
                        break
                    if received_at > deadline:      # arrived after the recording window
                        break
                    writer.writerow([getattr(packet, name) for name in FIELDS])
                    rows += 1
            except asyncio.CancelledError:
                aborted = True
                raise
        # leaving the `with` closed (and saved) the file

    finally:
        if aborted:
            log(f"{NAME} aborted, {rows} rows saved to {path}")
        await _send_stop_commands(link, log)

    if rows == 0:
        log(f"{NAME} no control debug packets received. "
            f"Is the debug output set to the control packet?")
    else:
        log(f"{NAME} done, {rows} rows saved to {path}")
