#!/usr/bin/env python3
"""Preprocess the actual Darwin policy under controlled platform/deployment combinations."""
import argparse
from pathlib import Path
import subprocess
import tempfile


def main():
    """Check exclusion boundaries without pretending to compile against an Apple SDK."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    common = ["__APPLE__=1", "__MACH__=1", "__aarch64__=1"]
    cases = [
        ("default-off", common, 1, "0 0"),
        (
            "modern-opt-in",
            common
            + [
                "FVM_ENABLE_MACOS_ARM64_JIT=1",
                "__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__=110400",
            ],
            1,
            "1 1",
        ),
        (
            "old-deployment",
            common
            + [
                "FVM_ENABLE_MACOS_ARM64_JIT=1",
                "__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__=110300",
            ],
            1,
            "0 0",
        ),
        ("missing-deployment", common + ["FVM_ENABLE_MACOS_ARM64_JIT=1"], 1, "0 0"),
        (
            "explicit-off",
            common
            + [
                "FVM_ENABLE_MACOS_ARM64_JIT=0",
                "__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__=110400",
            ],
            1,
            "0 0",
        ),
        (
            "arm64e-excluded",
            common
            + [
                "FVM_ENABLE_MACOS_ARM64_JIT=1",
                "__arm64e__=1",
                "__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__=110400",
            ],
            1,
            "0 0",
        ),
        (
            "ilp32-excluded",
            common
            + [
                "FVM_ENABLE_MACOS_ARM64_JIT=1",
                "__ILP32__=1",
                "__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__=110400",
            ],
            1,
            "0 0",
        ),
        (
            "ios-excluded",
            common
            + [
                "FVM_ENABLE_MACOS_ARM64_JIT=1",
                "__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__=110400",
            ],
            0,
            "0 0",
        ),
        ("classic-intel", ["__APPLE__=1", "__MACH__=1", "__x86_64__=1"], 1, "1 0"),
        ("not-darwin", ["__aarch64__=1", "FVM_ENABLE_MACOS_ARM64_JIT=1"], 1, "0 0"),
    ]
    with tempfile.TemporaryDirectory(prefix="flashvm-macos-policy-") as temporary:
        folder = Path(temporary)
        source = folder / "policy.c"
        source.write_text(
            '#include "ports/mac/code_platform.h"\n' "FVM_MAC_CODE_SUPPORTED FVM_MAC_CODE_ARM64\n",
            encoding="utf-8",
        )
        for name, definitions, desktop, expected in cases:
            (folder / "TargetConditionals.h").write_text(
                "#define TARGET_OS_OSX %d\n" % desktop, encoding="utf-8"
            )
            command = [
                args.cc,
                "-E",
                "-P",
                "-x",
                "c",
                "-U__APPLE__",
                "-U__MACH__",
                "-U__x86_64__",
                "-U__aarch64__",
                "-U__arm64__",
                "-U__arm64e__",
                "-U__ILP32__",
                "-U__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__",
                "-I" + str(folder),
                "-I" + str(root),
            ]
            command += ["-D" + definition for definition in definitions]
            output = subprocess.check_output(command + [str(source)], text=True).strip()
            if output != expected:
                raise RuntimeError("%s: expected %s, got %s" % (name, expected, output))
            print("PASS", name)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
