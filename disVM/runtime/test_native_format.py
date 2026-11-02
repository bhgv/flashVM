"""Apply real stdout backpressure to print/fprint with independently checked UTF-8 bytes."""
from pathlib import Path
import os
import fcntl
import re
import selectors
import subprocess
import sys
import tempfile
import time

# Expected UTF-8 comes from an independent Python construction, not host formatting.
PAYLOAD = b"".join(("Ж:"+"y"*2048+":"+str(i)+"\n").encode()*2 for i in range(64))  # Exact expected stdout bytes.
FILE_BYTES = b"".join(("row:"+str(i)+":Ж\n").encode() for i in range(64))  # Exact expected file bytes.
READY = b"NATIVE FORMAT ready\n"  # Emitted before genuine stdout backpressure.



# Run actual VM/native output. runtime/module are matching build products; mode,
# flags and quantum choose execution ownership. A finite budget expects managed
# unwind after native I/O. Return instruction count for that later error control.
def execute(runtime, module, mode, flags, quantum, budget=None):
    command = [str(runtime), mode, "--stats", *flags, "--quantum", str(quantum),
               "--max-instructions", str(budget if budget is not None else 10000000), str(module)]
    environment = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy",
                       DVM_EDITOR_THREAD_STATS="1")
    with tempfile.TemporaryDirectory(prefix="native format ") as directory:
        root = Path(directory)
        (root/"input.txt").write_bytes(b"input stays immutable")
        process = subprocess.Popen(command, cwd=directory, env=environment,
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        try:
            # Restrict only this private pipe, never a global kernel setting.
            try:
                fcntl.fcntl(process.stdout.fileno(),fcntl.F_SETPIPE_SZ,4096)
            except OSError:
                pass
            if fcntl.fcntl(process.stdout.fileno(),fcntl.F_GETPIPE_SZ) >= len(PAYLOAD):
                raise RuntimeError("stdout pipe is too large to prove backpressure")
            with selectors.DefaultSelector() as selector:
                selector.register(process.stderr,selectors.EVENT_READ)
                if not selector.select(30):
                    raise RuntimeError("native formatr did not reach handshake")
                marker = process.stderr.readline()
            if marker != READY:
                raise RuntimeError("native format setup failed: "+repr(marker))
            # Source is ready. Leave stdout undrained so an actual positive write
            # blocks on a full OS pipe while the GC owns its saved managed roots.
            time.sleep(0.05)
            output, diagnostic = process.communicate(timeout=45)
        except BaseException:
            process.kill()
            process.communicate()
            raise
        text = (diagnostic+output).decode(errors="replace")
        expected = 0 if budget is None else 1
        if process.returncode != expected:
            raise RuntimeError(text[-5000:])
        if budget is None:
            if (not output.startswith(PAYLOAD+b"PASS native format:") or
                    (root/"formatted.txt").read_bytes() != FILE_BYTES or
                    (root/"input.txt").read_bytes() != b"input stays immutable"):
                raise RuntimeError("exact output/file bytes or immutable input changed")
        elif "instruction limit exceeded" not in text or "PASS Dis application completed" in text:
            raise RuntimeError("missing output error retirement:\n"+text[-5000:])
        resources = re.search(r"NATIVEFORMATTED pins=0 calls=(\d+) releases=(\d+)", text)
        if not resources or int(resources[1]) < 195:
            raise RuntimeError("formatted native attempts/root retirement failed:\n"+text[-5000:])
        if "--gc-worker" in flags and resources[1] != resources[2]:
            raise RuntimeError("not every formatted port attempt released ownership")
        if "--gc-worker" not in flags and int(resources[2]):
            raise RuntimeError("inline reference unexpectedly released heap ownership")
        if "--gc-worker" in flags:
            if not re.search(r"serial_heap=1 admissions=[1-9]\d* contentions=\d+ waiters=0", text):
                raise RuntimeError("production exclusive heap admission not drained")
            counters = re.search(r"native_releases=(\d+) native_reentries=(\d+) native_gc_progress=(\d+)", text)
            if (not counters or not int(counters[1]) or counters[1] != counters[2] or
                    not int(counters[3]) or "pending=0" not in text or "joined=1 failed=0" not in text):
                raise RuntimeError("native format/GC/retirement failed:\n"+text[-5000:])
        if runtime.name == "disvm-editor" and not re.search(
                r"main_dis_tls=0 worker_bound=1 worker_released=1 result="+str(expected), text):
            raise RuntimeError("UI/Dis writer owner retirement failed:\n"+text[-5000:])
        instructions = re.search(r"instructions=(\d+)",text)
        if not instructions:
            raise RuntimeError("missing real instruction counter")
        return int(instructions[1])


# Exercise all collector routes and both dispatch modes. Reference runs determine
# a near-completion instruction limit; the error control must join after real I/O.
def main():
    runtime, module = (Path(value).resolve() for value in sys.argv[1:3])
    for mode in ("--jit", "--no-jit"):
        work = None
        for flags in (("--gc-inline",), ("--gc-worker",),
                      ("--gc-worker", "--gc-locked-poll")):
            for quantum in (31, 2000):
                work = execute(runtime,module,mode,flags,quantum)
        execute(runtime,module,mode,("--gc-worker",),31,budget=work-1)
    print("PASS native format: 12 exact-byte file/backpressure/root runs, 2 managed-error joins; "
          "inline/atomic/locked, interpreter/JIT, UTF-8 variadic formatting, saved roots")


if __name__ == "__main__":
    main()
