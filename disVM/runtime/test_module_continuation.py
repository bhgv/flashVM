"""Hold a real native pipe read across a second guest's call into the same module."""
import os
from pathlib import Path
import re
import selectors
import subprocess
import sys
import tempfile
import time


def run(runtime, directory, mode, poll, payload, outcome, image="main.dis", shared_channels=False, default_profile=False, workers=2, dedicated=False):
    shared = ["--parallel-channel-buffer", "--parallel-byte-copies"] if shared_channels else []
    explicit = [] if default_profile else ["--gc-worker", "--managed-workers", "2", "--guarded-copies", *shared]
    if default_profile and workers != 2:
        explicit = ["--managed-workers", str(workers)]
    if dedicated:
        explicit = ["--native-spawn", "--guarded-copies"]
    command = [str(runtime), mode, "--stats", *explicit, "--quantum", "65536", *poll, image]
    with subprocess.Popen(command, cwd=directory, stdin=subprocess.PIPE,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                          bufsize=0) as child:
        try:
            output = bytearray()
            peer = b"PEER released provider\n"
            deadline = time.monotonic() + 15
            with selectors.DefaultSelector() as events:
                events.register(child.stdout, selectors.EVENT_READ)
                while peer not in output:
                    if not events.select(max(0, deadline - time.monotonic())):
                        raise RuntimeError("peer did not release provider: " + repr(output))
                    chunk = os.read(child.stdout.fileno(), 4096)
                    if not chunk:
                        raise RuntimeError("runtime exited before peer release: " + repr(output))
                    output.extend(chunk)
            suspended = b"NATIVEWAITERS inflight=1\n"
            if suspended not in output or output.index(suspended) > output.index(peer):
                raise RuntimeError("peer ran before native suspension: " + repr(output))
            rest, _ = child.communicate(payload, timeout=15)
            output.extend(rest)
        finally:
            if child.poll() is None:
                child.kill()
                child.communicate()
    text = output.decode(errors="replace")
    pool = f"NATIVEPOOL executors={workers} joined=1 failed=0" in text
    if dedicated:
        starts = re.findall(r"NATIVEGUEST start pid=(\d+) thread=(\d+)", text)
        retired = re.findall(r"NATIVEGUEST retired pid=(\d+) thread=(\d+)", text)
        expected = 2 if image == "kill.dis" else 43
        concurrent = 2 if image == "kill.dis" else 40
        # The last forty channel guests all exist before the parent sends to any.
        pool = (len(starts) == expected and len(set(pid for pid, _ in starts)) == expected
                and sorted(starts) == sorted(retired)
                and re.search(r"NATIVEDISPATCH .* active=0 migrated=0\b", text) is not None
                and len(set(thread for _, thread in sorted(starts, key=lambda pair: int(pair[0]))[-concurrent:])) == concurrent)
    queue = re.search(r"NATIVEQUEUE queued=0 leases=(\d+) returns=(\d+) inflight=0", text)
    dispatch = re.search(r"NATIVEDISPATCH claims=(\d+) finishes=(\d+) active=0", text)
    gc = re.search(r"NATIVEGC collector=worker passes=(\d+).*?joined=1 failed=0", text)
    channel = re.search(r"parallel_channel_sends=(\d+) parallel_channel_receives=(\d+)", text)
    alts = re.search(r"parallel_channel_alts=(\d+)", text)
    rendezvous = re.search(r"parallel_rendezvous=(\d+)", text)
    raw = re.search(r"parallel_channel_raw=(\d+)", text)
    byte_moves = re.search(r"parallel_byte_commits=(\d+)", text)
    native = re.search(r"instructions=\d+ native=(\d+)", text)
    if (child.returncode or not pool or outcome not in text
            or (image == "main.dis" and "PASS forty independent channels" not in text) or "PASS Dis application completed" not in text
            or not queue or int(queue[1]) == 0 or queue[1] != queue[2]
            or not dispatch or dispatch[1] != dispatch[2] or not gc or int(gc[1]) == 0
            or ((shared_channels or default_profile) and (not channel or min(map(int, channel.groups())) == 0
                                         or not alts or int(alts[1]) != 2
                                         or not rendezvous or int(rendezvous[1]) != 2
                                         or not raw or int(raw[1]) < 2
                                         or not byte_moves or int(byte_moves[1]) < 2))
            or not native or (int(native[1]) > 0) != (mode == "--jit")):
        raise RuntimeError(text)


def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    fixtures = Path(__file__).resolve().parent / "fixtures/module-continuation"
    with tempfile.TemporaryDirectory(prefix="module-continuation-") as temporary:
        directory = Path(temporary)
        for name in ("provider", "main", "kill"):
            subprocess.run([str(compiler), "-F", "-I", str(include), "-I", str(fixtures),
                            "-I", str(fixtures.parents[2] / "module"),
                            "-o", str(directory / (name + ".dis")),
                            str(fixtures / (name + ".b"))], check=True)
        run(runtime, directory, "--jit", [], b"R", "PASS provider return",
            default_profile=True, workers=24)
        for mode in ("--jit", "--no-jit"):
            run(runtime, directory, mode, [], b"R", "PASS provider return", dedicated=True)
            run(runtime, directory, mode, [], b"R", "PASS provider termination and reload",
                "kill.dis", dedicated=True)
            for poll in ([], ["--gc-locked-poll"]):
                run(runtime, directory, mode, poll, b"R", "PASS provider return")
                run(runtime, directory, mode, poll, b"E", "PASS provider unwind")
                run(runtime, directory, mode, poll, b"R", "PASS provider return", shared_channels=True)
                run(runtime, directory, mode, poll, b"E", "PASS provider unwind", shared_channels=True)
                run(runtime, directory, mode, poll, b"R", "PASS provider return", default_profile=True)
                run(runtime, directory, mode, poll, b"R", "PASS provider termination and reload", "kill.dis")


if __name__ == "__main__":
    main()
