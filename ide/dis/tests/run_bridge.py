#!/usr/bin/env python3
"""Execute a compiled bridge/index fixture in an explicitly selected matching reference runtime."""
import argparse
from pathlib import Path
import subprocess


# No emulator/SDK discovery, network or fallback occurs in this optional reference acceptance.
# CLI supplies emulator, build namespace, module and mode; failures raise with captured diagnostics.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--emu", required=True, type=Path)
    parser.add_argument("--build", required=True, type=Path)
    parser.add_argument(
        "--module",
        required=True,
        choices=("idebridge_test", "idecomplete_test", "idebuiltins_test", "ideworkspace_test"),
    )
    parser.add_argument("--mode", required=True, choices=("0", "1"))
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
            "/" + args.module + ".dis",
        ],
        cwd=root,
        capture_output=True,
        timeout=20,
    )
    text = result.stdout.decode(errors="replace")
    errors = result.stderr.decode(errors="replace")
    prefix = "PASS " + args.module[:-5] + " "
    if result.returncode or prefix not in text or "FAIL" in text or "Broken:" in text + errors:
        raise RuntimeError(
            "reference execution failed; use a matching compiler/header/runtime ABI:\n"
            + text
            + "\n"
            + errors
        )
    print(text.strip())


if __name__ == "__main__":
    main()
