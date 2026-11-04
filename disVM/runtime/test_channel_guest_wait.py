"""Exercise guarded channel contention on two production Dis workers."""
from pathlib import Path
import subprocess
import sys
import tempfile

from relink_private import relink


SEND_GUEST = r'''implement ChannelWait;
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


RECEIVE_GUEST = r'''implement ChannelWait;
include "sys.m";
ChannelWait: module { init: fn(); };

receive(messages, output: chan of int)
{
    output <-= <-messages;
}

init()
{
    sys := load Sys Sys->PATH;
    messages := chan[3] of int;
    output := chan[1] of int;
    messages <-= 11;
    messages <-= 31;
    spawn receive(messages, output);
    first := <-messages;
    second := <-output;
    if (!((first == 11 && second == 31) || (first == 31 && second == 11)))
        raise "channel receive payload mismatch";
    sys->print("PASS guest channel payloads\n");
}
'''


FAILURE_GUEST = r'''implement ChannelWait;
include "sys.m";
ChannelWait: module { init: fn(); };

attempt(messages: chan of int, value: int): int
{
    {
        messages <-= value;
        return 1;
    } exception {
        "fixture channel mover failure" => return 0;
    }
}

send(messages, output: chan of int)
{
    output <-= attempt(messages, 11);
}

init()
{
    sys := load Sys Sys->PATH;
    messages := chan[4] of int;
    output := chan[1] of int;
    spawn send(messages, output);
    sent := attempt(messages, 31);
    other := <-output;
    if (sent + other != 1)
        raise "expected one successful send";
    value := <-messages;
    if ((sent && value != 31) || (other && value != 11))
        raise "failed send published a payload";
    sys->print("PASS guest channel payloads\n");
}
'''


def main():
    """Use the actual allocator and wait path with an injected slow mover."""
    runtime, compiler, include = (Path(arg).resolve() for arg in sys.argv[1:4])
    with tempfile.TemporaryDirectory(prefix="channel-guest-wait-") as directory:
        work = Path(directory)
        binary = relink(runtime, work, Path(__file__).with_suffix(".c").resolve(), 0,
                        ["dvm_runtime_channel_new", "dvm_runtime_channel_wait_guest"])
        for name, guest in (("send", SEND_GUEST), ("receive", RECEIVE_GUEST),
                            ("failure", FAILURE_GUEST)):
            source, image = work / f"{name}.b", work / f"{name}.dis"
            source.write_text(guest)
            subprocess.run([compiler, "-I", include, "-o", image, source], check=True)
            for mode in ("--no-jit", "--jit"):
                result = subprocess.run([
                    binary, mode, "--guarded-copies", "--gc-worker", "--managed-workers", "2", image,
                ], capture_output=True, text=True, timeout=30)
                output = result.stdout + result.stderr
                if (result.returncode or output.count("PASS foreign channel guest wait") != 1
                        or "PASS guest channel payloads" not in output
                        or "ThreadSanitizer:" in output or "runtime error:" in output):
                    raise RuntimeError(f"{name} {mode}\n{output}")
    print("PASS guarded channel contention on two native Dis workers in C and JIT modes")


if __name__ == "__main__":
    main()
