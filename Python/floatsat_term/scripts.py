"""`script <name>`: send a user-defined list of commands from sequences.py."""
import asyncio
import runpy
from pathlib import Path

SEQUENCES_FILE = Path(__file__).with_name("sequences.py")
COMMAND_SPACING_S = 0.25


class SequenceError(Exception):
    pass


def load_sequences() -> dict[str, list[str]]:
    """Read sequences.py from disk (every call, so edits apply right away)."""
    try:
        namespace = runpy.run_path(str(SEQUENCES_FILE))
    except FileNotFoundError:
        raise SequenceError(f"{SEQUENCES_FILE} not found")
    except Exception as e:      # syntax error or anything else raised by the file
        raise SequenceError(f"could not load {SEQUENCES_FILE.name}: {type(e).__name__}: {e}")

    sequences = namespace.get("SEQUENCES")
    if not isinstance(sequences, dict):
        raise SequenceError(f"{SEQUENCES_FILE.name} must define SEQUENCES as a dict")
    for name, commands in sequences.items():
        if not isinstance(name, str) or not isinstance(commands, list) \
                or not all(isinstance(c, str) for c in commands):
            raise SequenceError(f"sequence {name!r} must be a list of strings")
    return sequences


async def run(name: str, commands: list[str], link, log):
    tag = f"[script {name}]"
    commands = [c.strip() for c in commands if c.strip()]
    log(f"{tag} {len(commands)} commands  ('abort' to stop)")
    sent = 0
    try:
        for i, command in enumerate(commands):
            if i > 0:
                await asyncio.sleep(COMMAND_SPACING_S)
            log(f"{tag} ({i + 1}/{len(commands)}) {command}")
            await link.send_command(command)
            sent += 1
        log(f"{tag} done")
    except asyncio.CancelledError:
        log(f"{tag} aborted after {sent}/{len(commands)} commands")
        raise
