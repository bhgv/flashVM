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
    assert before.groups()[:2] == after.groups()[:2] == ('3', '10') and before[4] == after[4]
    assert Decimal(after[3])-Decimal(before[3]) == Decimal('3.20')
    assert on.stdout.splitlines()[1:] == off.stdout.splitlines()[1:]
    for option, expected in [('--tracking', '53.00'), ('--condensed', '44.00'), ('--no-ligatures', '24.00'), ('--ligature-tracking', '26.00')]:
        tracked = subprocess.run([sys.argv[1], option], capture_output=True, text=True, check=True)
        actual = re.match(pattern, tracked.stdout)
        assert actual and actual.groups()[:3] == ('1', '2', expected)
    underlined = subprocess.run([sys.argv[1], '--underline'], capture_output=True, text=True, check=True)
    assert underlined.stdout.splitlines()[:-2] == on.stdout.splitlines()
    assert underlined.stdout.splitlines()[-2:] == [
        'underline center=4.00 thickness=2.00 commands=15', 'underline probe_alpha=255']
    justified = subprocess.run([sys.argv[1], '--justify'], capture_output=True, text=True, check=True)
    actual = re.match(pattern, justified.stdout)
    assert actual and actual.groups() == ('2', '7', '90.00', '92.00')
    assert justified.stdout.splitlines()[-1] == 'justify first=90.00 last=60.00'
    bad = subprocess.run([sys.argv[1], '--unknown'], capture_output=True, text=True)
    assert bad.returncode != 0 and not bad.stdout and 'usage:' in bad.stderr
    print('PASS C89 explicit kerning: analytical AV delta, preserved ligature/bidi lines signed tracking, underline contours/pixel oracle and actual RGBA rasterization')
    return 0


if __name__ == '__main__':
    sys.exit(main())
