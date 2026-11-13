# Copyright (c) 2026 bhgv
# EN: Real typed owner callbacks, live aliases, timeout and owner-retirement acceptance.
# RU: Реальные typed owner callbacks, live aliases, timeout и owner retirement.
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from resident_evidence import evidence
from relink_private import relink


# EN: Compile matching owner library/handler/main into a private runtime directory.
# CLI inputs select runtime/compiler/includes. Require exact user callback, native
# lifetime and concurrent-GC evidence in C/JIT with both register polling paths.
# RU: Собрать owner library/handler/main в private runtime directory. CLI выбирает
# runtime/compiler/includes. Проверить callback/lifetime/concurrent GC в C/JIT
# с обоими register poll paths. Files/children owned through completion.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    positive = sys.argv[4:] == ["--positive-only"]
    if sys.argv[4:] and not positive:
        raise RuntimeError("unsupported guest proxy test arguments")
    root = Path(__file__).resolve().parent
    module = root.parent / "module"
    with tempfile.TemporaryDirectory(prefix="guest owner proxy ") as temporary:
        work = Path(temporary)
        for source, name in (
            (root.parent / "limbo/proxyowner.b", "proxyowner"),
            (root / "fixtures/proxy-owner/handler.b", "handler"),
            (root / "fixtures/proxy-owner/main.b", "main"),
        ):
            compiled = subprocess.run(
                [
                    str(compiler),
                    "-I",
                    str(include),
                    "-I",
                    str(module),
                    "-o",
                    str(work / (name + ".dis")),
                    str(source),
                ],
                text=True,
                capture_output=True,
            )
            if compiled.returncode:
                raise RuntimeError(
                    "guest proxy compile failed\n" + compiled.stdout + compiled.stderr
                )
        guarded = relink(
            runtime, work, root / "test_channel_gc_lock.c", 0, ["dvm_proxyhost_initialize"]
        )
        for mode in ("--no-jit", "--jit"):
            for locked in (False, True):
                command = [
                    str(guarded),
                    "--stats",
                    mode,
                    *(["--gc-locked-poll"] if locked else []),
                    str(work / "main.dis"),
                ]
                try:
                    result = subprocess.run(
                        command, cwd=work, capture_output=True, text=True, timeout=90
                    )
                except subprocess.TimeoutExpired as error:
                    output = "".join(
                        part.decode(errors="replace") if isinstance(part, bytes) else part
                        for part in (error.stdout, error.stderr)
                        if part
                    )
                    raise RuntimeError(
                        f"guest proxy timeout {mode} locked={locked}\n{output[-30000:]}"
                    ) from error
                output = result.stdout + result.stderr
                if (
                    result.returncode
                    or "PASS GC quiet observation refuses occupied queue" not in output
                    or "ThreadSanitizer:" in output
                    or "runtime error:" in output
                    or output.count(
                        "PASS guest owner proxies: variables functions refs aliases deadlines independent-owner kill cleanup"
                    )
                    != 1
                    or output.count("PROXYHOST opened=4 closed=4 exports=0 active=0") != 1
                ):
                    raise RuntimeError(f"{mode} locked={locked}\n{output[-16000:]}")
                children = len(re.findall(r"NATIVEGUEST start pid=", output))
                if children < 12:
                    raise RuntimeError("real owners/clients/alarms did not execute\n" + output)
                evidence(output, children=children)
                if not re.search(
                    r"NATIVERC enqueued=(\d+) released=\1 pending=0 saturated=0 bytes=0 byte_limited=0",
                    output,
                ):
                    raise RuntimeError("proxy roots or callback return owners leaked\n" + output)
        if not positive:
            for variant, symbol in (
                (1, "fvm_port_sync_try_lock"),
                (2, "dvm_rc_queued_record_group"),
            ):
                broken_queue = relink(
                    runtime,
                    work,
                    root / "test_channel_gc_lock.c",
                    variant,
                    ["dvm_proxyhost_initialize", symbol],
                )
                refused = subprocess.run(
                    [str(broken_queue), "--no-jit", str(work / "main.dis")],
                    cwd=work,
                    capture_output=True,
                    text=True,
                    timeout=15,
                )
                if (
                    refused.returncode != 86
                    or "FAIL GC queue observation blocked or lost retry" not in refused.stderr
                ):
                    raise RuntimeError(
                        f"queue control {variant} escaped\n" + refused.stdout + refused.stderr
                    )
            source = (root / "proxyhost_call.c").read_text()
            pattern = r"\(uint64_t\)session != dvm_proxyhost_state.identity \|\|\s*"
            mutated, count = re.subn(pattern, "", source)
            if count != 1:
                raise RuntimeError("guest session fence control anchor changed")
            replacement = work / "old-session.c"
            replacement.write_text(mutated)
            wrapper = work / "empty.c"
            wrapper.write_text("/* Copyright (c) 2026 bhgv; private empty link control. */\n")
            broken = relink(runtime, work, wrapper, 0, [], proxyhost_call_source=replacement)
            result = subprocess.run(
                [str(broken), "--stats", "--no-jit", str(work / "main.dis")],
                cwd=work,
                capture_output=True,
                text=True,
                timeout=30,
            )
            output = result.stdout + result.stderr
            if (
                result.returncode != 1
                or "old session handle resolved a replacement root" not in output
            ):
                raise RuntimeError("missing session fence was not detected\n" + output)
    print(
        "PASS guest proxy API: actual variable/function owner, live ref aliases, 8 clients, "
        "deadlines, callback errors, independent owner and killed active callback; C/JIT and both polls"
    )
    if not positive:
        print("PASS blocking GC queue observation and lost capture retry rejected")
        print("PASS omitted session fence rejected before replacement root use")


if __name__ == "__main__":
    main()
