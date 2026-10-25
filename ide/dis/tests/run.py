"""Run the real Limbo channel regression in an explicitly selected reference emulator."""

import argparse
from pathlib import Path
import re
import subprocess


# Use a build-owned root and direct init module, without a shell, Draw context or boot scripts.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--emu", required=True, type=Path)
    parser.add_argument("--build", required=True, type=Path)
    parser.add_argument("--mode", required=True, choices=("0", "1"))
    parser.add_argument(
        "--module",
        default="idecore_test.dis",
        choices=(
            "idecore_test.dis",
            "limits_test.dis",
            "revision_test.dis",
            "coordinates_test.dis",
            "find_test.dis",
            "navigation.dis",
            "history.dis",
            "history_test.dis",
            "historylimits_test.dis",
            "replaceall_test.dis",
            "replaceall.dis",
            "utf8span_test.dis",
        ),
    )
    args = parser.parse_args()
    root = args.build.resolve()
    for name in ("dev", "chan", "prog", "fd", "net", "net.alt", "env"):
        (root / name).mkdir(exist_ok=True)
    result = subprocess.run(
        [
            str(args.emu.resolve()),
            "-c" + args.mode,
            "-r" + str(root),
            "-d",
            "/" + args.module,
        ],
        cwd=root,
        capture_output=True,
        timeout=30,
    )
    output = result.stdout.decode("utf-8", errors="replace")
    if not (result.returncode == 0):
        raise AssertionError((result.returncode, output, result.stderr))
    if not (
        "FAIL" not in output
        and re.search(
            "^PASS idecore (?:(?:limits|revision|coordinates|find|navigation|history|historylimits|historyexample|replaceall|replaceallexample|utf8span) )?\\d+ checks$",
            output,
            re.MULTILINE,
        )
    ):
        raise AssertionError((output, result.stderr))
    print(output.strip())


if __name__ == "__main__":
    main()
