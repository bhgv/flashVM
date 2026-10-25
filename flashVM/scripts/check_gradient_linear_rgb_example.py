#!/usr/bin/env python3
"""Check actual RGB/linearRGB fill and stroke pixels while retaining legacy sample output."""
import subprocess
import sys


def main():
    """Require four measured RGBA midpoints, or explicitly skip a backend-free executable."""
    result = subprocess.run([sys.argv[1]], capture_output=True, text=True, check=True)
    if "Select GRAPHICS_AGG or GRAPHICS_CAIRO" in result.stdout:
        print("SKIP linearRGB tutorial requires a raster backend")
        return 77
    expected = [
        "center RGBA: 124 0 131 255",
        "stroke pad: 128 0 0",
        "stroke reflect: 128 0 255",
        "stroke repeat: 128 255 255",
        "linear fill RGB: 128 128 128 128",
        "linear fill linearRGB: 188 188 188 128",
        "linear stroke RGB: 128 128 128 128",
        "linear stroke linearRGB: 188 188 188 128",
    ]
    if result.stdout.splitlines()[: len(expected)] != expected:
        raise RuntimeError("Unexpected gradient midpoints: " + repr(result.stdout))
    print("PASS C89 RGB/linearRGB fill/stroke midpoint colors and unchanged straight alpha")
    return 0


if __name__ == "__main__":
    sys.exit(main())
