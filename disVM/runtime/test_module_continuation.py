"""Hold a real native pipe read across a second guest's call into the same module."""
import argparse
import os
from pathlib import Path
import re
import selectors
import subprocess
import tempfile
import time

from relink_private import relink


def run(runtime, directory, mode, poll, payload, outcome, image="main.dis", shared_channels=False, default_profile=False, workers=2, dedicated=False):
    shared = ["--parallel-channel-buffer", "--parallel-byte-copies"] if shared_channels else []
    explicit = [] if default_profile else ["--gc-worker", "--managed-workers", "2", "--guarded-copies", *shared]
    if default_profile and workers != 2:
        explicit = ["--managed-workers", str(workers)]
    if dedicated:
        explicit = ["--native-spawn", "--guarded-copies", *shared]
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


def portable_overlap(runtime, compiler, include, runner):
    """Run the same NC64 guest on a target console without relinking host objects."""
    fixtures = Path(__file__).resolve().parent / "fixtures/module-continuation"
    with tempfile.TemporaryDirectory(prefix="module-continuation-target-") as temporary:
        image = Path(temporary) / "overlap.dis"
        for name in ("provider", "overlap"):
            subprocess.run([str(compiler), "-F", "-I", str(include), "-I", str(fixtures),
                            "-o", str(Path(temporary) / (name + ".dis")),
                            str(fixtures / (name + ".b"))], check=True)
        result = subprocess.run(
            [*runner, str(runtime), "--no-jit", "--native-spawn", "--stats", str(image)],
            cwd=temporary, capture_output=True, text=True, timeout=120,
        )
        output = result.stdout + result.stderr
        batches = re.search(r"shared_integer_batches=(\d+)", output)
        widths = re.search(r"shared_byte_steps=(\d+) shared_long_steps=(\d+)", output)
        pointer_copies = re.search(r"parallel_movp_module=(\d+)", output)
        frames = re.search(r"shared_calls=(\d+) shared_returns=(\d+)", output)
        module_frames = re.search(r"shared_module_calls=(\d+) shared_module_returns=(\d+)", output)
        starts = re.findall(r"NATIVEGUEST start pid=(\d+) thread=(\d+)", output)
        retired = re.findall(r"NATIVEGUEST retired pid=(\d+) thread=(\d+)", output)
        dispatch = re.search(r"NATIVEDISPATCH claims=(\d+) finishes=(\d+) active=0 migrated=0", output)
        gc = re.search(r"NATIVEGC collector=worker passes=(\d+).*?joined=1 failed=0", output)
        queue = re.search(r"NATIVEQUEUE queued=0 leases=(\d+) returns=(\d+) inflight=0", output)
        frame_retirement = re.search(r"NATIVEFRAMERC drained=(\d+) pending=0", output)
        retirement = re.search(r"NATIVERC enqueued=(\d+) released=(\d+) pending=0", output)
        if (result.returncode or "PASS forty dedicated shared copies" not in output
                or "PASS Dis application completed" not in output
                or len(starts) != 40 or len({pid for pid, _ in starts}) != 40
                or len({thread for _, thread in starts}) != 40 or sorted(starts) != sorted(retired)
                or not pointer_copies or int(pointer_copies[1]) == 0
                or not frames or min(map(int, frames.groups())) < 40
                or not module_frames or min(map(int, module_frames.groups())) < 40
                or not batches or int(batches[1]) < 40
                or not widths or min(map(int, widths.groups())) < 40
                or not dispatch or dispatch[1] != dispatch[2]
                or not gc or int(gc[1]) == 0
                or not queue or queue[1] == "0" or queue[1] != queue[2]
                or not retirement or retirement[1] == "0" or retirement[1] != retirement[2]
                or not frame_retirement or int(frame_retirement[1]) < 40):
            raise RuntimeError(output)
        print(output, end="")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("runtime", "compiler", "include"):
        parser.add_argument(name, type=lambda value: Path(value).resolve())
    parser.add_argument("--portable-overlap", action="store_true")
    parser.add_argument("--runner", nargs=argparse.REMAINDER, default=[])
    args = parser.parse_args()
    runtime, compiler, include = args.runtime, args.compiler, args.include
    if args.portable_overlap:
        portable_overlap(runtime, compiler, include, args.runner)
        return
    if args.runner:
        parser.error("--runner requires --portable-overlap")
    fixtures = Path(__file__).resolve().parent / "fixtures/module-continuation"
    with tempfile.TemporaryDirectory(prefix="module-continuation-") as temporary:
        directory = Path(temporary)
        for name in ("provider", "main", "kill", "overlap"):
            subprocess.run([str(compiler), "-F", "-I", str(include), "-I", str(fixtures),
                            "-I", str(fixtures.parents[2] / "module"),
                            "-o", str(directory / (name + ".dis")),
                            str(fixtures / (name + ".b"))], check=True)
        overlap = relink(runtime, directory, fixtures / "overlap.c", 0,
                         ["dvm_native_owner_enter_shared", "dvm_native_owner_try_share",
                          "dvm_gc_workers_request", "dvm_runtime_rc_transfer_frame",
                          "dvm_runtime_exception_pop"])
        for mode in ("--jit", "--no-jit"):
            for options, counter in ((["--guarded-copies", "--parallel-movp", "--parallel-slicela"], "parallel_slicela_commits"),
                                     ([], "shared_integer_commits")):
                result = subprocess.run(
                    [str(overlap), mode, "--native-spawn", *options,
                     "--stats", str(directory / "overlap.dis")],
                    cwd=directory, capture_output=True, text=True, timeout=30,
                )
                output = result.stdout + result.stderr
                commits = re.search(rf"{counter}=(\d+)", output)
                peak = re.search(r"shared_peak=(\d+)", output)
                branches = re.search(r"shared_integer_branches=(\d+)", output)
                widths = re.search(r"shared_byte_steps=(\d+) shared_long_steps=(\d+)", output)
                batches = re.search(r"shared_integer_batches=(\d+)", output)
                pointer_copies = re.search(r"parallel_movp_module=(\d+)", output)
                frames = re.search(r"shared_calls=(\d+) shared_returns=(\d+)", output)
                frame_retirement = re.search(r"NATIVEFRAMERC drained=(\d+) pending=0", output)
                module_frames = re.search(r"shared_module_calls=(\d+) shared_module_returns=(\d+)", output)
                if (result.returncode or "PASS forty dedicated shared copies" not in output
                        or not pointer_copies or int(pointer_copies[1]) == 0
                        or (not options and (output.count("FRAME exception popped in shared mode") < 40
                            or "FRAME retirement retry injected" not in output
                            or not frame_retirement or int(frame_retirement[1]) < 40
                            or not frames or min(map(int, frames.groups())) < 40
                            or not module_frames or min(map(int, module_frames.groups())) < 40
                            or not batches or int(batches[1]) < 40
                            or not branches or int(branches[1]) < 5120
                            or not widths or min(map(int, widths.groups())) < 40))
                        or not peak or int(peak[1]) < 2 or not commits or int(commits[1]) < 40
                        or not re.search(r"NATIVEDISPATCH .* active=0 migrated=0", output)):
                    raise RuntimeError(f"{mode} {options}\n{output}")
        run(runtime, directory, "--jit", [], b"R", "PASS provider return",
            default_profile=True, workers=24)
        for mode in ("--jit", "--no-jit"):
            run(runtime, directory, mode, [], b"R", "PASS provider return", dedicated=True)
            run(runtime, directory, mode, [], b"E", "PASS provider unwind",
                shared_channels=True, dedicated=True)
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
