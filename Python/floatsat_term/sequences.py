"""User-defined command sequences, run from the prompt with `script <name>`.

Each entry maps a name to a list of commands. The commands are sent to the
FloatSat exactly as written, in order, with at least
scripts.COMMAND_SPACING_S (0.25 s) between them.

This file is read again every time `script` is typed, so edits take effect
without restarting the terminal. Type `script` alone to list the names.
"""

SEQUENCES = {
    "example": [
        "control -a 0",
        "control -e",
    ],
    "test1":[
        "mission 2",
        "sys --debug-id 3",
        "photo add -a 0 -n 1 -t 00:00:30",
        "photo add -a 0 -n 1 -t 00:00:32",
        "photo add -a 0 -n 1 -t 00:02:10",
        "photo add -a 0 -n 1 -t 00:03:35",
    ]
}
