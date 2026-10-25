#!/usr/bin/env python3
"""Observe actual C89 shaping/path rasterization with original analytical GPOS AV geometry."""
import re
import subprocess
import sys
from decimal import Decimal


def main():
    """Require stable bidi/cluster lines and the independently specified -80/1000 em pair delta."""
    on = subprocess.run([sys.argv[1]], capture_output=True, text=True, check=True)
    if "requires FONTS" in on.stdout:
        print("SKIP tutorial profile has no native font backend")
        return 77
    off = subprocess.run([sys.argv[1], "--no-kerning"], capture_output=True, text=True, check=True)
    pattern = r"lines=(\d+) glyphs=(\d+) advance=([\d.]+) height=([\d.]+)"
    before, after = re.match(pattern, on.stdout), re.match(pattern, off.stdout)
    if not (before and after):
        raise AssertionError(
            "condition check failed: flashVM/scripts/check_text_kerning_example.py:18"
        )
    if not (before.groups()[:2] == after.groups()[:2] == ("3", "10") and before[4] == after[4]):
        raise AssertionError(
            "condition check failed: flashVM/scripts/check_text_kerning_example.py:19"
        )
    if not (Decimal(after[3]) - Decimal(before[3]) == Decimal("3.20")):
        raise AssertionError(
            "condition check failed: flashVM/scripts/check_text_kerning_example.py:20"
        )
    if not (on.stdout.splitlines()[1:] == off.stdout.splitlines()[1:]):
        raise AssertionError(
            "condition check failed: flashVM/scripts/check_text_kerning_example.py:21"
        )
    for option, expected in [
        ("--tracking", "53.00"),
        ("--condensed", "44.00"),
        ("--no-ligatures", "24.00"),
        ("--ligature-tracking", "26.00"),
    ]:
        tracked = subprocess.run([sys.argv[1], option], capture_output=True, text=True, check=True)
        actual = re.match(pattern, tracked.stdout)
        if not (actual and actual.groups()[:3] == ("1", "2", expected)):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_text_kerning_example.py:30"
            )
    underlined = subprocess.run(
        [sys.argv[1], "--underline"], capture_output=True, text=True, check=True
    )
    if not (underlined.stdout.splitlines()[:-2] == on.stdout.splitlines()):
        raise AssertionError(
            "condition check failed: flashVM/scripts/check_text_kerning_example.py:34"
        )
    if not (
        underlined.stdout.splitlines()[-2:]
        == ["underline center=4.00 thickness=2.00 commands=15", "underline probe_alpha=255"]
    ):
        raise AssertionError(
            "condition check failed: flashVM/scripts/check_text_kerning_example.py:35"
        )
    justified = subprocess.run(
        [sys.argv[1], "--justify"], capture_output=True, text=True, check=True
    )
    actual = re.match(pattern, justified.stdout)
    if not (actual and actual.groups() == ("2", "7", "90.00", "92.00")):
        raise AssertionError(
            "condition check failed: flashVM/scripts/check_text_kerning_example.py:43"
        )
    if not (justified.stdout.splitlines()[-1] == "justify first=90.00 last=60.00"):
        raise AssertionError(
            "condition check failed: flashVM/scripts/check_text_kerning_example.py:44"
        )
    for option, advance, origin in [
        ("--indent", "102.00", "30.00"),
        ("--hanging-indent", "90.00", "-18.00"),
    ]:
        indented = subprocess.run([sys.argv[1], option], capture_output=True, text=True, check=True)
        actual = re.match(pattern, indented.stdout)
        if not (actual and actual.groups() == ("2", "7", advance, "92.00")):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_text_kerning_example.py:51"
            )
        if not (indented.stdout.splitlines()[-1] == "indent first_x=" + origin + " next_x=0.00"):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_text_kerning_example.py:52"
            )
    tabbed = subprocess.run([sys.argv[1], "--tabs"], capture_output=True, text=True, check=True)
    actual = re.match(pattern, tabbed.stdout)
    if not (actual and actual.groups() == ("1", "5", "104.00", "44.00")):
        raise AssertionError(
            "condition check failed: flashVM/scripts/check_text_kerning_example.py:55"
        )
    if not (tabbed.stdout.splitlines()[-1] == "tabs first_stop=40.00 second_stop=80.00"):
        raise AssertionError(
            "condition check failed: flashVM/scripts/check_text_kerning_example.py:56"
        )
    periodic = subprocess.run(
        [sys.argv[1], "--default-tabs"], capture_output=True, text=True, check=True
    )
    actual = re.match(pattern, periodic.stdout)
    if not (actual and actual.groups() == ("1", "5", "199.00", "44.00")):
        raise AssertionError(
            "condition check failed: flashVM/scripts/check_text_kerning_example.py:61"
        )
    if not (periodic.stdout.splitlines()[-1] == "default-tabs first_stop=87.50 second_stop=175.00"):
        raise AssertionError(
            "condition check failed: flashVM/scripts/check_text_kerning_example.py:62"
        )
    bad = subprocess.run([sys.argv[1], "--unknown"], capture_output=True, text=True)
    if not (bad.returncode != 0 and (not bad.stdout) and ("usage:" in bad.stderr)):
        raise AssertionError(
            "condition check failed: flashVM/scripts/check_text_kerning_example.py:64"
        )
    print(
        "PASS C89 explicit kerning: analytical AV delta, preserved ligature/bidi lines signed tracking, underline contours/pixel oracle and actual RGBA rasterization"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
