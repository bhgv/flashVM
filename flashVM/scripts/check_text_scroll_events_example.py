#!/usr/bin/env python3
"""Require real C89 callback order and final committed TextField viewport state."""
import subprocess
import sys


def main():
    result = subprocess.run(
        [sys.argv[1], "--scroll-events"], capture_output=True, text=True, check=True
    )
    if "requires FONTS" in result.stdout:
        print("SKIP no-font scroll event tutorial")
        return 77
    if not (
        result.stdout.splitlines()[-3:]
        == [
            "scroll events: 1 2 1 2 1 1 1 1 1 1 1",
            "scroll event bottoms: 2 2 2 2 3 3 3 1 1 2 1",
            "scroll events state: top=1 horizontal=0 max=2/40 bottom=1",
        ]
    ):
        raise AssertionError(result.stdout)
    print(
        "PASS C89 field-first scroll callbacks, no-op suppression, removal, geometry/text/style ranges, bottom-only delivery and final state"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
