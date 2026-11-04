"""Exercise guarded channel contention on two production Dis workers."""
from pathlib import Path
import subprocess
import sys
import tempfile

from relink_private import relink


GUEST = r'''implement ChannelWait;
include "sys.m";
ChannelWait: module { init: fn(); };

send(other: chan of int)
{
    other <-= 11;
}

init()
{
    sys := load Sys Sys->PATH;
    messages := chan[2] of int;
    spawn send(messages);
    messages <-= 31;
    first := <-messages;
    second := <-messages;
    if (!((first == 11 && second == 31) || (first == 31 && second == 11)))
        raise "channel payload mismatch";
    sys->print("PASS guest channel payloads\n");
}
'''


def main():
    """Use the actual allocator and wait path with an injected slow mover."""
    runtime, compiler, include = (Path(arg).resolve() for arg in sys.argv[1:4])
    with tempfile.TemporaryDirectory(prefix="channel-guest-wait-") as directory:
        work = Path(directory)
        source, image = work / "main.b", work / "main.dis"
        source.write_text(GUEST)
        subprocess.run([compiler, "-I", include, "-o", image, source], check=True)
        binary = relink(runtime, work, Path(__file__).with_suffix(".c").resolve(), 0,
                        ["dvm_runtime_channel_new", "dvm_runtime_channel_wait_guest"])
        for mode in ("--no-jit", "--jit"):
            result = subprocess.run([
                binary, mode, "--guarded-copies", "--gc-worker", "--managed-workers", "2", image,
            ], capture_output=True, text=True, timeout=30)
            output = result.stdout + result.stderr
            if (result.returncode or output.count("PASS foreign channel guest wait") != 1
                    or "PASS guest channel payloads" not in output
                    or "ThreadSanitizer:" in output or "runtime error:" in output):
                raise RuntimeError(output)
    print("PASS guarded channel contention on two native Dis workers in C and JIT modes")


if __name__ == "__main__":
    main()
