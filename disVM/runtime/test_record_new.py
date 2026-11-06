"""Exercise guarded record, array and channel construction through native and guest routes."""
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
    items := array[3] of ref Pair;
    items[2] = item;
    if (items[0] != nil || items[2] != item) { raise "array initialization"; }
    if (item.name != "record" || item.value != 73) {
        raise "record initialization";
    }
    words := chan[2] of int;
    words <-= 29;
    if (<-words != 29) { raise "word channel"; }
    pairs := chan[2] of (string, int);
    pairs <-= ("mapped", 42);
    (label, number) := <-pairs;
    if (label != "mapped" || number != 42) { raise "mapped channel"; }
    raw := chan[2] of (int, int);
    raw <-= (10, 20);
    (first, second) := <-raw;
    if (first != 10 || second != 20) { raise "raw channel"; }
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
            "dvm_runtime_root_registry_probe", "dvm_runtime_record_new", "dvm_runtime_heap_commit",
            "dvm_runtime_array_new", "dvm_runtime_root_batch_publish", "destroy", "fvm_port_sync_wait", "dvm_managed_workers_wake_guest", "dvm_managed_ranges_begin"
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
                "PASS detached ALT publication failure", "PASS native channel buffer wait", "PASS rooted channel buffer lifetime", "PASS buffered channel completion", "PASS direct channel mover failure", "PASS two-sided channel finalization", "PASS native Prog hold race", "PASS guarded channel drain", "PASS mapped channel receive ownership", "PASS mapped channel send publication", "PASS guarded mapped channel mover", "PASS guarded channels", "PASS guarded arrays", "PASS array opcode route", "PASS rooted record construction", "PASS imported record lease", "PASS record opcode route", "PASS record guest"
            )) or "ThreadSanitizer:" in output or "runtime error:" in output:
                raise RuntimeError(output)
    print("PASS guarded records, arrays and channels, stale writer, refusal, monitor error and C/JIT guest")


if __name__ == "__main__":
    main()
