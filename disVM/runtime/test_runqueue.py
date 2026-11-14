# Copyright (c) 2026 bhgv
# EN: Actual runqueue owner/lease controls across split native modules.
# RU: Проверки владельцев и native-ожиданий на настоящих модулях очереди.
"""Compile the real production queue and exercise semantic ownership controls."""

from pathlib import Path
import os
import re
import sys
import shlex
import subprocess
import tempfile


# Use actual retained Prog declarations and real native ports. Private temporary
# units contain intentional controls only; repository sources remain untouched.
# No args/result; compiler/semantic/timeout errors raise instead of counting pass.
def main():
    positive_only = sys.argv[1:] == ["--positive-only"]
    if sys.argv[1:] and not positive_only:
        raise RuntimeError("expected optional --positive-only")
    root = Path(__file__).resolve().parents[2]
    runtime = root / "disVM/runtime"
    with tempfile.TemporaryDirectory(prefix="runqueue ownership ") as directory:
        work = Path(directory)
        command = shlex.split(os.environ.get("CC", "cc"))
        command += shlex.split(os.environ.get("CFLAGS", "-O1 -g"))
        command += ["-pthread", "-DDVM_RUNTIME_PROCESS_MAX=128"]
        for include in (
            runtime,
            root / "disVM/host",
            root / "disVM/host/include",
            root / "disVM/upstream/include",
            root / "disVM/upstream/libinterp",
            root / "disVM/upstream/emu/port",
            root / "disVM/include",
            root / "flashVM/ports",
        ):
            command += ["-I" + str(include)]
        command += [
            str(runtime / "test_runqueue.c"),
            str(runtime / "runqueue.c"),
            str(runtime / "runqueue_native.c"),
            str(runtime / "process_lifetime.c"),
            str(runtime / "owner_messages.c"),
            str(root / "disVM/host/native_context.c"),
            str(root / "disVM/host/refcount.c"),
            str(root / "flashVM/ports/linux/thread.c"),
            str(root / "flashVM/ports/linux/channel_sync.c"),
            "-o",
            str(work / "queue"),
        ]
        subprocess.run(command, check=True)
        subprocess.run([command[-1]], check=True, timeout=30)
        if positive_only:
            print("PASS real runqueue and readiness publication; omission controls excluded")
            return
        source = (runtime / "runqueue.c").read_text()
        controls = [
            (
                "__atomic_store_n(observer, state, __ATOMIC_RELAXED);",
                "/* private control: omit diagnostic mirror */",
                "native observation mirror diverged",
            ),
            (
                "!table_wake && runtime_process(process)->dispatch_owner &&",
                "runtime_process(process)->dispatch_owner &&",
                "early table wake refused",
            ),
            (
                "    __atomic_fetch_add(&returns, 1, __ATOMIC_RELAXED);",
                "    /* private control: omit return accounting */",
                "native return accounting diverged",
            ),
            (
                "return !process || !process->native64holds;",
                "(void)process; return 1;",
                "synchronous Native64Caller hold was promoted to resumable",
            ),
            (
                "runtime_process(process)->native_owner != context ||\n        runtime_process(process)->dispatch_owner != context ||",
                "",
                "foreign continuation restoration accepted",
            ),
            (
                "runtime_process(process)->dispatch_owner || dvm_native_context_process(context) ||",
                "dvm_native_context_process(context) ||",
                "foreign managed dispatch claim accepted",
            ),
            (
                "if (!context || !process || runtime_process(process)->dispatch_owner != context ||",
                "if (!context || !process ||",
                "foreign managed dispatch finish accepted",
            ),
            (
                "ready_count() + __atomic_load_n(&inflight, __ATOMIC_RELAXED) >= DVM_RUNTIME_PROCESS_MAX || runtime_process(process)->queued ||",
                "ready_count() + __atomic_load_n(&inflight, __ATOMIC_RELAXED) >= DVM_RUNTIME_PROCESS_MAX ||",
                "duplicate runnable admission accepted",
            ),
            (
                "ready_count() + __atomic_load_n(&inflight, __ATOMIC_RELAXED) >= DVM_RUNTIME_PROCESS_MAX",
                "ready_count() >= DVM_RUNTIME_PROCESS_MAX",
                "native return slot was not reserved",
            ),
            (
                "    if (dvm_runqueue_pop_expected(process, Prelease) != process)\n    {\n        return 0;\n    }",
                "",
                "native continuation remained runnable",
            ),
        ]
        context_path = root / "disVM/host/native_context.c"
        context_source = context_path.read_text()
        native_markers = {
            "native return accounting diverged",
            "synchronous Native64Caller hold was promoted to resumable",
            "foreign continuation restoration accepted",
            "native continuation remained runnable",
        }
        updated = []
        for before, after, marker in controls:
            if marker == "early table wake refused":
                before, after = "!table_wake && dispatch &&", "dispatch &&"
            for old, new in (
                (
                    "runtime_process(process)->dispatch_owner",
                    "dvm_runqueue_dispatch_owner(process)",
                ),
                ("runtime_process(process)->native_owner", "dvm_runqueue_owner(process)"),
                ("&returns", "&dvm_runqueue_leases.returns"),
                ("&inflight", "&dvm_runqueue_leases.inflight"),
                ("ready_count()", "dvm_runqueue_ready_count()"),
                ("runtime_process(process)->queued", "dvm_runqueue_is_ready(process)"),
            ):
                before, after = before.replace(old, new), after.replace(old, new)
            unit = runtime / ("runqueue_native.c" if marker in native_markers else "runqueue.c")
            updated.append((unit, unit.read_text(), before, after, marker))
        controls = updated
        controls += [
            (
                context_path,
                context_source,
                "|| !token || context->turn)",
                "|| !token)",
                "cleared GC root permitted nested dispatch",
            ),
            (
                context_path,
                context_source,
                "context->heap_owner.world || context->turn)",
                "context->heap_owner.world)",
                "claimed context unbound before finish",
            ),
        ]
        for index, (unit, original, before, after, marker) in enumerate(controls):
            pattern = re.compile(r"\s+".join(re.escape(piece) for piece in before.split()))
            if len(list(pattern.finditer(original))) != 1:
                raise RuntimeError("ownership control predicate not unique: " + marker)
            altered = work / ("control" + str(index) + ".c")
            altered.write_text(pattern.sub(lambda unused: after, original, count=1))
            altered_command = [str(altered) if item == str(unit) else item for item in command]
            if marker == "foreign managed dispatch finish accepted":
                # Exact ownership has TWO independent checks: the shared Prog
                # claim and its context-local native token. Remove both in the
                # private control; either intact layer must refuse the attack.
                needle = "|| !token || context->turn != token)"
                if context_source.count(needle) != 1:
                    raise RuntimeError("context release control target changed")
                weak_context = work / "weak_release_context.c"
                weak_context.write_text(context_source.replace(needle, "|| !token)"))
                altered_command = [
                    str(weak_context) if item == str(context_path) else item
                    for item in altered_command
                ]
            altered_command[-1] = str(work / ("control" + str(index)))
            subprocess.run(altered_command, check=True)
            result = subprocess.run(
                [altered_command[-1]], capture_output=True, text=True, timeout=30
            )
            if result.returncode != 1 or marker not in result.stderr:
                raise RuntimeError(
                    "inactive ownership control: " + marker + "\n" + result.stdout + result.stderr
                )
    print(
        "PASS twelve active ownership/observation controls: native mirror, early table wake, return accounting, synchronous hold, foreign restore/claim/finish, "
        "duplicate admission, reserved return slot, missing withdrawal, nested cleared-root turn, early unbind"
    )


if __name__ == "__main__":
    main()
