#!/usr/bin/env python3
"""Inspect one immutable Dis64 image; never load or execute it."""
from pathlib import Path
import argparse
import json
import sys

# Use the same complete Python closure from checkout or installed SDK. No peer
# checkout, network request, manifest local declarations or native VM is needed.
HERE = Path(__file__).resolve()  # Location of this documented example.
SOURCE = HERE.parents[2]/'tests'  # Checkout: disVM/envelope/tests.
if not (SOURCE/'peer_call_semantics.py').is_file():
    SOURCE = HERE.parents[3]/'source/dis64-checkpoint'  # Installed SDK closure.
sys.path.insert(0, str(SOURCE))
from peer_call_semantics import inspect_image, MAX_IMAGE


def main():
    """Read the explicit CLI path with a byte ceiling and print its staged JSON.

    Return0 means inspection completed, NOT execution admitted. Invalid input I/O
    or hex raises an argparse diagnostic. No output file or runtime state is made.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('image', type=Path, help='.dis64 bytes or .hex text')
    args = parser.parse_args()
    try:
        # Read at most one byte above the ceiling to detect a growing/oversized
        # file without publishing a misleading SHA of only a truncated prefix.
        limit = MAX_IMAGE*4 if args.image.suffix == '.hex' else MAX_IMAGE
        with args.image.open('rb') as stream:
            data = stream.read(limit+1)
        if len(data) > limit:
            parser.error('example input exceeds its offline byte ceiling')
        if args.image.suffix == '.hex':
            data = bytes.fromhex(data.decode('ascii'))
    except (OSError, ValueError, UnicodeError) as failure:
        parser.error(str(failure))
    # The result always keeps owning_execution_supported false, including when
    # every implemented bounded rule passes. Actual loader/GC/code life is separate.
    print(json.dumps(inspect_image(data), indent=2))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
