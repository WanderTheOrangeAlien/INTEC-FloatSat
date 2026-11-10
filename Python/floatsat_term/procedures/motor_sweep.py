"""`test motor`: step the reaction wheel PWM and log raw gyro Z.

Starting at 0 and going up to 1 in steps of 0.05, for each PWM value:
  1. send `motor pwm <val>`
  2. record the next SAMPLES IMU debug packets, one CSV row each
  3. go straight to the next value (no settling time, no averaging)

Each row is (timestamp_ms, pwm_val, gyro_z). timestamp_ms is the time the
packet arrived, in ms since the test started. A step lasts SAMPLES divided by
the IMU debug rate (200 samples at 100 Hz = 2 s).

Each step is flushed to disk when it finishes, and the file is closed on every
exit path, so an aborted run keeps what it recorded. The motor is always set
back to 0 at the end, including when the test is aborted, fails, or the
console exits mid-test.
"""
import asyncio
import csv
from datetime import datetime
from pathlib import Path

from ..protocol import ImuDebug

PWM_START = 0.0
PWM_STOP = 1.0
PWM_STEP = 0.1
SAMPLES = 300                   # IMU debug packets recorded per PWM value
SAMPLE_TIMEOUT_S = 5.0          # give up if no debug packet arrives for this long
RESULTS_DIR = Path("results")

NAME = "[test motor]"


def pwm_values() ->list[float]:
    n = round((PWM_STOP - PWM_START) / PWM_STEP)
    res = [None] * 4*n
    for i in range(n):
        res[4*i] = round(PWM_START + i * PWM_STEP, 4)
        res[4*i + 1] = 0
        res[4*i + 2] = -round(PWM_START + i * PWM_STEP, 4)
        res[4*i + 3] = 0
    return res


def _drain(queue: asyncio.Queue):
    while not queue.empty():
        queue.get_nowait()


async def run(link, log):
    RESULTS_DIR.mkdir(exist_ok=True)
    path = RESULTS_DIR / f"motor_sweep_{datetime.now():%Y%m%d_%H%M%S}.csv"
    values = pwm_values()
    log(f"{NAME} {len(values)} steps x {SAMPLES} samples -> {path}  ('abort' to stop)")

    loop = asyncio.get_running_loop()
    rows = 0

    with path.open("w", newline="") as f, link.subscribe(ImuDebug) as imu:
        writer = csv.writer(f)
        writer.writerow(["timestamp_ms", "pwm_val", "gyro_z"])
        f.flush()
        t0 = loop.time()
        try:
            for pwm in values:
                # Throw away packets received before the command goes out, so
                # every recorded sample is from after it.
                _drain(imu)
                await link.send_command(f"motor --pwm {pwm:.2f}")

                for _ in range(SAMPLES):
                    try:
                        received_at, packet = await asyncio.wait_for(imu.get(), SAMPLE_TIMEOUT_S)
                    except asyncio.TimeoutError:
                        log(f"{NAME} no IMU debug packet for {SAMPLE_TIMEOUT_S:.0f} s at pwm "
                            f"{pwm:.2f}, stopping. Is the debug output enabled?")
                        return
                    writer.writerow([packet.timestamp, pwm,
                                     round(packet.gyro.z, 6)])
                    rows += 1
                f.flush()
                log(f"{NAME} pwm {pwm:.2f} done ({rows} rows)")

            log(f"{NAME} done, {rows} rows saved to {path}")

        except asyncio.CancelledError:
            log(f"{NAME} aborted, {rows} rows saved to {path}")
            raise

        finally:
            try:
                await link.send_command("motor --pwm 0.00")
                log(f"{NAME} motor set back to 0")
            except Exception as e:
                log(f"{NAME} could not send 'motor pwm 0.00' ({type(e).__name__}). "
                    f"STOP THE MOTOR MANUALLY")
