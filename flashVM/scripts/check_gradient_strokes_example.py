#!/usr/bin/env python3
"""Check actual C89 stroke pixels printed at the half, first and second ramp phases."""
import subprocess
import sys


def main():
    """Require backend samples, while treating the documented NO_GRAPHICS path as a skip."""
    result = subprocess.run([sys.argv[1]], capture_output=True, text=True, check=True)
    if 'Select GRAPHICS_AGG or GRAPHICS_CAIRO' in result.stdout:
        print('SKIP gradient stroke tutorial requires a raster backend')
        return 77
    expected = [
        'center RGBA: 124 0 131 255',
        'stroke pad: 128 0 0',
        'stroke reflect: 128 0 255',
        'stroke repeat: 128 255 255',
    ]
    if result.stdout.splitlines() != expected:
        raise RuntimeError('Unexpected gradient samples: ' + repr(result.stdout))
    print('PASS C89 PAD/REFLECT/REPEAT stroke phase samples and legacy fill')
    return 0


if __name__ == '__main__':
    sys.exit(main())
