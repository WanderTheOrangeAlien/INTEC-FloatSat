"""Scripted test procedures, run from the prompt with `test <name>`.

A procedure is an async function `run(link, log)`:
  link  the Link: `await link.send_command("...")`, `with link.subscribe(PacketClass) as q:`
  log   function that prints a line to the console log

While a procedure runs, typing `abort` cancels it (asyncio.CancelledError is
raised inside it at its current await), so put any cleanup in a `finally`.
To add one, write a module here and register its run() below.
"""
from . import angle_log, control_log, motor_sweep

PROCEDURES = {
    "motor": motor_sweep.run,
    "angle": angle_log.run,
    "control": control_log.run,
}
