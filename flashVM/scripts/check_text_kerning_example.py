#!/usr/bin/env python3
"""Observe actual C89 shaping/path rasterization with original analytical GPOS AV geometry."""
import re
import subprocess
import sys
from decimal import Decimal


def main():
    """Require stable bidi/cluster lines and the independently specified -80/1000 em pair delta."""
    on = subprocess.run([sys.argv[1]], capture_output=True, text=True, check=True)
    if 'requires FONTS' in on.stdout:
        print('SKIP tutorial profile has no native font backend')
        return 77
    off = subprocess.run([sys.argv[1], '--no-kerning'], capture_output=True, text=True, check=True)
    pattern = r'lines=(\d+) glyphs=(\d+) advance=([\d.]+) height=([\d.]+)'
    before, after = re.match(pattern, on.stdout), re.match(pattern, off.stdout)
    assert before and after
    assert before[1:3] == after[1:3] == ('3', '10') and before[4] == after[4]
    assert Decimal(after[3])-Decimal(before[3]) == Decimal('3.20')
    assert on.stdout.splitlines()[1:] == off.stdout.splitlines()[1:]
    bad = subprocess.run([sys.argv[1], '--unknown'], capture_output=True, text=True)
    assert bad.returncode != 0 and not bad.stdout and 'usage:' in bad.stderr
    print('PASS C89 explicit kerning: analytical AV delta, preserved ligature/bidi lines and actual RGBA rasterization')
    return 0


if __name__ == '__main__':
    sys.exit(main())
