#!/usr/bin/env python3
"""Check the actual C89 scroll viewport and contour demonstration against analytic face dimensions."""
import subprocess
import sys


def main():
    """Observe actual exported bounds, not echoed requested translation."""
    result = subprocess.run([sys.argv[1], '--scroll'], capture_output=True, text=True, check=True)
    if 'requires FONTS' in result.stdout:
        print('SKIP no-font viewport demonstration')
        return 77
    assert result.stdout.splitlines()[-3:] == [
        'scroll: top=2 bottom=3 max=2 horizontal=7 maxHorizontal=12',
        'scroll viewport: x=2 y=2 width=36 height=90',
        'scroll path: dx=-7 dy=-46'], result.stdout
    bad = subprocess.run([sys.argv[1], '--scroll', '--wrong'], capture_output=True)
    assert bad.returncode != 0
    print('PASS C89 scroll geometry and actual owned contour translation')
    return 0


if __name__ == '__main__':
    sys.exit(main())
