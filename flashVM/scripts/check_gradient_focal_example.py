#!/usr/bin/env python3
"""Check actual signed interior focal pixels while retaining legacy gradient sample output."""
import subprocess
import sys


def main():
    """Require signed focal axis samples, or explicitly skip a backend-free executable."""
    result = subprocess.run([sys.argv[1]], capture_output=True, text=True, check=True)
    if "Select GRAPHICS_AGG or GRAPHICS_CAIRO" in result.stdout:
        print("SKIP focal tutorial requires a raster backend")
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
        "focal fill -0.5: 255 170 85",
        "focal fill 0: 128 255 128",
        "focal fill 0.5: 85 170 255",
        "focal stroke -0.5: 255 170 85",
        "focal stroke 0: 128 255 128",
        "focal stroke 0.5: 85 170 255",
    ]
    if result.stdout.splitlines() != expected:
        raise RuntimeError("Unexpected focal samples: " + repr(result.stdout))
    print("PASS C89 signed focal axis samples and all legacy gradient outputs")
    return 0


if __name__ == "__main__":
    sys.exit(main())
