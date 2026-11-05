"""Usage: python -m floatsat_term <ip[:port]>"""
import asyncio
import sys

from .app import main

if len(sys.argv) != 2:
    print("usage: python -m floatsat_term <ip[:port]>")
    sys.exit(2)

try:
    sys.exit(asyncio.run(main(sys.argv[1])))
except KeyboardInterrupt:
    pass
