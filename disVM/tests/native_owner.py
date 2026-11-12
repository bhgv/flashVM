#!/usr/bin/env python3
# Copyright (c) 2026 bhgv
"""EN: Check concurrent native owners, roots, epoch sampling and registration.
RU: Проверка параллельных владельцев, корней, эпох и регистрации.
"""

import argparse
import os
from pathlib import Path
from world_source import read_world_source
import shlex
import subprocess
import tempfile
from native_registers import instruction


# Parse the explicit FlashVM source root and build in owned temporary storage.
# No arguments/result; production instruction/world sources are borrowed unchanged.
# Compiler, process, timeout or semantic failures raise; only joined positive
# runs and the specific deliberate negative-control failure count as success.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--flashvm-root", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    fvm = args.flashvm_root.resolve()
    code = (
        (root / "tests/native_owner.c")
        .read_text()
        .replace(
            "PRODUCTION_ADDW", instruction((root / "upstream/libinterp/xec.c").read_text(), "addw")
        )
        .replace("PRODUCTION_DRIVER", (root / "tests/native_owner_driver.inc").read_text())
    )
    with tempfile.TemporaryDirectory(prefix="disvm-native-owner-") as directory:
        output = Path(directory)
        source = output / "world.c"
        source.write_text(code)
        command = shlex.split(os.environ.get("CC", "cc"))
        command += shlex.split(os.environ.get("CFLAGS", "-O1 -g"))
        command += ["-DDVM_NATIVE_REGISTERS=1", "-pthread"]
        command += [
            "-I" + str(path)
            for path in [
                root / "host/include",
                root / "upstream/include",
                root / "upstream/libinterp",
                root / "upstream/emu/port",
                root / "include",
                fvm / "include",
                fvm / "ports",
                root / "examples/native-owner",
            ]
        ]
        command += [
            str(source),
            str(root / "examples/native-owner/native_wait.c"),
            str(root / "host/native_registers.c"),
            str(root / "host/native_context.c"),
            str(root / "host/native_owner.c"),
            str(root / "host/native_world.c"),
            str(root / "host/flashvm_thread.c"),
            str(fvm / "ports/linux/thread.c"),
            str(fvm / "ports/linux/channel_sync.c"),
            "-o",
            str(output / "test"),
        ]
        subprocess.run(command, check=True)
        for mode in ("atomic", "locked"):
            subprocess.run([str(output / "test"), mode], check=True, timeout=30)
        # Reject a deliberately restored global mutator exclusion predicate.
        # The peer must fail on entry while the first owner is still active.
        original = read_world_source(root / "host")
        predicate = "else if (participant->active || (shared && (!world->serial || world->sealed)))"
        if original.count(predicate) != 1:
            raise RuntimeError("native entry predicate is not unique")
        control_source = output / "world-control.c"
        control_source.write_text(
            original.replace(
                predicate,
                "else if (participant->active || (!shared && world->active) || "
                "(shared && (!world->serial || world->sealed)))",
            )
        )
        control_binary = output / "control"
        control_command = [
            str(control_source) if item == str(root / "host/native_world.c") else item
            for item in command
        ]
        control_command[-1] = str(control_binary)
        control_command.insert(1, "-I" + str(root / "host"))
        subprocess.run(control_command, check=True)
        result = subprocess.run(
            [str(control_binary), "atomic"], capture_output=True, text=True, timeout=30
        )
        if not result.returncode or "concurrent peer admission" not in result.stderr:
            raise RuntimeError(
                "inactive mutator-exclusion control: " + result.stdout + result.stderr
            )
        owner_source = (root / "host/native_owner.c").read_text()
        snapshot = "    owner->before = epoch;"
        if owner_source.count(snapshot) != 1:
            raise RuntimeError("native owner epoch snapshot is not unique")
        altered = output / "owner-control.c"
        altered.write_text(owner_source.replace(snapshot, "    (void)epoch;"))
        alternate = [
            str(altered) if item == str(root / "host/native_owner.c") else item for item in command
        ]
        alternate.insert(1, "-I" + str(root / "host"))
        alternate[-1] = str(output / "epoch-control")
        subprocess.run(alternate, check=True)
        result = subprocess.run(
            [alternate[-1], "atomic"], capture_output=True, text=True, timeout=30
        )
        if not result.returncode or "owner epoch/call counters" not in result.stderr:
            raise RuntimeError(
                "inactive epoch-snapshot removal control: " + result.stdout + result.stderr
            )
        sealed = "if (world->sealed || world->collector || !dvm_native_context_set_poll"
        if original.count(sealed) != 1:
            raise RuntimeError("registration seal predicate is not unique")
        control_source.write_text(
            original.replace(sealed, "if (world->collector || !dvm_native_context_set_poll")
        )
        subprocess.run(control_command, check=True)
        result = subprocess.run(
            [str(control_binary), "atomic"], capture_output=True, text=True, timeout=30
        )
        if not result.returncode or "sealed owner registration reopened" not in result.stderr:
            raise RuntimeError(
                "inactive registration-seal control: " + result.stdout + result.stderr
            )
    print("PASS global exclusion control: simultaneous native owners are required")
    print("PASS owner leases: atomic/locked, epoch snapshot and registration-seal removal controls")


if __name__ == "__main__":
    main()
