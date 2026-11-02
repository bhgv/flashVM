"""Feed genuine delayed OS pipes to a whole managed read/readn application."""
from pathlib import Path
import os
import re
import subprocess
import sys
import threading
import tempfile
import time

LARGE = bytes((index*131 + (index >> 8)*17 + (index >> 16)*29) & 255
              for index in range(100003))  # Nonuniform binary data crosses both stage boundaries.
PAYLOAD = b"ABCDEFGHi" + LARGE + b"tail"  # Exact managed-stream contents.
ECHO = b"NATIVE READ ready\n" + LARGE + b"\nPASS native read:"  # Every echoed byte is checked.


# Own the native pipe writer on this Python OS thread. descriptor is closed on
# every path; errors is joined/read by the caller. Delays let a real consumer
# block, but elapsed time is not a correctness/performance oracle.
def feed(descriptor, errors):
    try:
        for fragment in (PAYLOAD[:2], PAYLOAD[2:5], PAYLOAD[5:]):
            time.sleep(0.03)
            offset = 0
            while offset < len(fragment):
                offset += os.write(descriptor, fragment[offset:])
    except OSError as failure:
        errors.append(failure)
    finally:
        os.close(descriptor)


# Run real interpreter/JIT and OS input. runtime/module are executable products;
# mode, flags and quantum select independent VM/GC variants. limited expects
# managed unwind after input; all ordinary runs must echo every byte exactly.
def execute_in(runtime, module, mode, flags, quantum, directory, limited=False):
    command = [str(runtime), mode, "--stats", "--quantum", str(quantum), *flags,
               "--max-instructions", "10000000" if not limited else "300"]
    command.append(str(module))
    read_end, write_end = os.pipe()
    errors = []
    environment = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy",
                       DVM_EDITOR_THREAD_STATS="1")
    try:
        process = subprocess.Popen(command, stdin=read_end, stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE, env=environment, cwd=directory)
    except BaseException:
        os.close(write_end)
        raise
    finally:
        os.close(read_end)
    writer = threading.Thread(target=feed, args=(write_end, errors))
    writer.start()
    try:
        output, diagnostic = process.communicate(timeout=45)
    except subprocess.TimeoutExpired:
        process.kill()
        process.communicate()
        raise
    finally:
        writer.join(timeout=5)
    expected = 1 if limited else 0
    text = (output+diagnostic).decode(errors="replace")
    if process.returncode != expected or writer.is_alive() or (errors and not limited):
        raise RuntimeError(text[-5000:]+repr(errors))
    if limited:
        if "instruction limit exceeded" not in text or "PASS Dis application completed" in text:
            raise RuntimeError("missing managed-error retirement:\n"+text[-5000:])
    elif not output.startswith(ECHO) or output.count(b"PASS native read:") != 1:
        raise RuntimeError("exact input/echo or managed assertions failed:\n"+text[-5000:])
    resources = re.search(r"NATIVEINPUT pins=0 staging=0 peak=(\d+) fragments=(\d+) copied=(\d+)", text)
    if not resources or int(resources[1]) > 65536:
        raise RuntimeError("native input roots/staging did not retire:\n"+text[-5000:])
    if not limited and (int(resources[1]) != 65536 or int(resources[3]) != 100032):
        raise RuntimeError("native input bounded-stage/commit accounting failed:\n"+text[-5000:])
    if "--gc-worker" in flags:
        counters = re.search(r"native_releases=(\d+) native_reentries=(\d+) native_gc_progress=(\d+)", text)
        if (not counters or not int(counters[1]) or counters[1] != counters[2] or
                (not limited and not int(counters[3])) or
                "pending=0" not in text or "joined=1 failed=0" not in text):
            raise RuntimeError("native input root/GC/join evidence failed:\n"+text[-5000:])
    if runtime.name == "disvm-editor" and not re.search(
            r"main_dis_tls=0 worker_bound=1 worker_released=1 result="+str(expected), text):
        raise RuntimeError("UI/Dis input retirement failed:\n"+text[-5000:])


# Keep generated regular-file inputs private and alive through child/thread joins.
# All execution parameters pass through unchanged; directory cleanup never touches
# user data or the module installation tree. Raises on any whole-process failure.
def execute(runtime, module, mode, flags, quantum, limited=False):
    with tempfile.TemporaryDirectory(prefix="native read ") as directory:
        root = Path(directory)
        (root/"input.bin").write_bytes(b"0123456789abcdef")
        (root/"empty.bin").write_bytes(b"")
        execute_in(runtime,module,mode,flags,quantum,directory,limited)


# Exercise actual pipes, partial reads, chunks, slices and lifetime through both
# dispatch modes and all GC routes; two failures test native resource unwinding.
def main():
    runtime, module = (Path(value).resolve() for value in sys.argv[1:3])
    for mode in ("--jit", "--no-jit"):
        for flags in (("--gc-inline",), ("--gc-worker",),
                      ("--gc-worker", "--gc-locked-poll")):
            for quantum in (31, 2000):
                execute(runtime,module,mode,flags,quantum)
        execute(runtime,module,mode,("--gc-worker",),31,limited=True)
    print("PASS native read: 12 exact-byte OS-pipe/root runs, 2 managed-error joins; "
          "inline/atomic/locked, interpreter/JIT, sliced and multi-stage input")


if __name__ == "__main__":
    main()
