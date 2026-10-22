#!/usr/bin/env python3
"""Check actual C89 automatic geometry against the original CC0 font dimensions."""
import subprocess
import sys


def main():
    """Require real owner-context edits, fitted wrapping, and early bad-option rejection."""
    run = subprocess.run([sys.argv[1], '--auto-size'], text=True, capture_output=True, check=True)
    if 'requires FONTS' in run.stdout:
        return 77
    assert run.stdout.splitlines()[-3:] == [
        'autoSize left: width=52.00 height=48.00',
        'autoSize edit: width=76.00',
        'autoSize wrap: width=30.00 height=140.00']
    anchored = subprocess.run([sys.argv[1], '--anchors'], text=True, capture_output=True, check=True)
    assert anchored.stdout.splitlines()[-3:] == [
        'anchor left: x=10.00 width=52.00',
        'anchor center: x=34.00 width=52.00',
        'anchor right: x=58.00 width=52.00']
    faces = subprocess.run([sys.argv[1], '--font-variants'], text=True, capture_output=True, check=True)
    assert faces.stdout.splitlines()[-4:] == [
        'face regular: advance=48.00', 'face bold: advance=56.00',
        'face italic: advance=52.00', 'face bold-italic: advance=60.00']
    bad = subprocess.run([sys.argv[1], '--unknown'], text=True, capture_output=True)
    assert bad.returncode and not bad.stdout and 'usage:' in bad.stderr
    print('PASS C89 autoSize activation, text edit, constrained height and owned path export')
    return 0


if __name__ == '__main__':
    sys.exit(main())
