#!/usr/bin/env python3
"""Run parallel GC admission with actual retained addw and OS owners."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
from native_registers import instruction


# Parse the explicit FlashVM source root and build in owned temporary storage.
# No arguments/result; production instruction/world sources are borrowed unchanged.
# Compiler, process, timeout and semantic failures raise. Both owners must join.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--flashvm-root", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    fvm = args.flashvm_root.resolve()
    code = (
        (root / "tests/native_serial.c")
        .read_text()
        .replace(
            "PRODUCTION_ADDW", instruction((root / "upstream/libinterp/xec.c").read_text(), "addw")
        )
    )
    with tempfile.TemporaryDirectory(prefix="disvm-native-serial-") as directory:
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
            ]
        ]
        command += [
            str(source),
            str(root / "host/native_registers.c"),
            str(root / "host/native_context.c"),
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
        # Reject overlapping ordinary entry in a private copy. The test must
        # detect this directly, without relying on a timeout.
        world = root / "host/native_world.c"
        original = world.read_text()
        anchor = "    if (try_only && (world->collector || world->resuming))"
        if original.count(anchor) != 1:
            raise RuntimeError("parallel admission control anchor changed")
        control = output / "world-control.c"
        control.write_text(original.replace(anchor, """
    if (!shared && world->active)
    {
        fvm_port_sync_unlock(world->sync);
        return 0;
    }
""" + anchor))
        control_command = [str(control) if item == str(world) else item for item in command]
        control_command.insert(1, "-I" + str(world.parent))
        subprocess.run(control_command, check=True)
        result = subprocess.run([str(output / "test"), "atomic"], capture_output=True,
                                text=True, timeout=30)
        if result.returncode == 0 or "enter while peer is active" not in result.stderr:
            raise RuntimeError("inactive parallel admission control: " + result.stdout + result.stderr)
    print("PASS parallel GC admission with atomic and locked polling")


if __name__ == "__main__":
    main()
