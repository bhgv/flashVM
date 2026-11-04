"""Exercise protected record construction and actual local NEW/NEWZ dispatch."""
from pathlib import Path
import subprocess
import sys
import tempfile

from relink_private import relink


GUEST = r'''implement Records;
include "sys.m";
Records: module { init: fn(); };
Pair: adt { name: string; value: int; };
init() {
    sys := load Sys Sys->PATH;
    item := ref Pair("record", 73);
    if (item.name != "record" || item.value != 73) {
        raise "record initialization";
    }
    sys->print("PASS record guest\n");
}
'''


def main():
    """Run real dispatch and rooted interleaving in both runtime profiles."""
    runtime, compiler, include = (Path(arg).resolve() for arg in sys.argv[1:4])
    fixture = Path(__file__).with_suffix(".c").resolve()
    with tempfile.TemporaryDirectory(prefix="record-new-") as directory:
        work = Path(directory)
        guest, image = work / "main.b", work / "main.dis"
        guest.write_text(GUEST)
        subprocess.run([str(compiler), "-I", str(include), "-o", str(image), str(guest)], check=True)
        binary = relink(runtime, work, fixture, 0, [
            "dvm_runtime_root_registry_probe", "dvm_runtime_record_new", "dvm_runtime_heap_commit"
        ])
        for mode in ("--no-jit", "--jit"):
            result = subprocess.run([
                str(binary), mode, "--guarded-copies", "--gc-worker",
                "--managed-owner-probe", str(image)
            ], capture_output=True, text=True, timeout=90)
            output = result.stdout + result.stderr
            if mode == "--jit" and "PASS guarded record JIT cache" not in output:
                raise RuntimeError(output)
            if result.returncode or any(marker not in output for marker in (
                "PASS rooted record construction", "PASS imported record lease", "PASS record opcode route", "PASS record guest"
            )) or "ThreadSanitizer:" in output or "runtime error:" in output:
                raise RuntimeError(output)
    print("PASS guarded record construction, stale writer, monitor error and C/JIT guest")


if __name__ == "__main__":
    main()
