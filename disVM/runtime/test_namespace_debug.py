# Copyright (c) 2026 bhgv
# EN: Inspect real shared/copied native namespaces while faults retain their source frames.
# RU: Исследовать реальные shared/copied namespaces при retained fault source frames.
from pathlib import Path
import re
import select
import subprocess
import sys
import tempfile

from debugger.client import PipeDebugger
from test_owner_location import compile_image
from test_retained_debug import require, retained
from test_debug_mailbox import verify
from relink_private import relink


# EN/RU: Read one observed guest line with deadline; no debugger traffic on stdout.
# Читать одну guest line с deadline; debugger не использует stdout.
def line(process, marker):
    require(select.select([process.stdout], [], [], 10)[0], "guest milestone timeout")
    value = process.stdout.readline()
    require(marker in value, "unexpected guest milestone: " + value)
    return value


# EN: Reconstruct one exact UTF-8 text from hex chunks at one revision, never
# decode partial Unicode sequences. No source pointer/address is accepted.
# RU: Собрать точный UTF-8 text из hex chunks одной revision; не декодировать
# частичные Unicode sequences. Native pointers/addresses не принимаются.
def text(session, pid, revision, field, index=0):
    result = bytearray()
    while True:
        row = session.command(f"namespace-read {pid} {revision} {index} {field} {len(result)}")
        require(row.get("ok"), "namespace text refusal: " + str(row))
        block = bytes.fromhex(row["hex"])
        require(len(block) == row["copied"] and len(block) <= 128, "invalid text bound")
        result.extend(block)
        if len(result) == row["length"]:
            return result.decode("utf-8"), row
        require(block and len(result) < row["length"], "namespace pagination made no progress")


# EN/RU: Execute four dispatch/poll choices; mutate only the shared group while
# both fault owners remain retained. Require stale refusal, copied isolation,
# strict parsing and complete normal retirement. Always own/clean child streams.
# Проверить оба dispatch/poll; менять shared group при retained fault owners.
# Требовать stale refusal, copy isolation, strict parsing и normal retirement.
def run(runtime, image, work, mode, locked, control=False):
    log = work / "errors.log"
    with log.open("w") as errors:
        session = PipeDebugger(
            runtime,
            [mode, "--stats", *(["--gc-locked-poll"] if locked else []), str(image)],
            cwd=work,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=errors,
            text=True,
        )
        process = session.process
        try:
            found = re.search(r"main=(\d+) shared=(\d+) copied=(\d+)", line(process, "NSDEBUG"))
            require(found, "missing owner identities")
            main, shared, copied = map(int, found.groups())
            retained(session, shared, "faulted")
            retained(session, copied, "faulted")
            require(
                session.command(f"namespace {main}").get("error") == "not retained",
                "running owner namespace was accessed",
            )
            a = session.command(f"namespace {shared}")
            b = session.command(f"namespace {copied}")
            require(
                a.get("ok") and b.get("ok") and a["bindings"] == 10 and b["bindings"] == 10,
                "namespace summary lost initial bindings",
            )
            require(
                all(a[name] and b[name] for name in ("root_held", "cwd_held", "import_held")),
                "missing held directories",
            )
            require(text(session, shared, a["revision"], 1)[0] == str(work), "shared cwd mismatch")
            require(
                text(session, copied, b["revision"], 1)[0] == str(work / "visible"),
                "copied cwd mismatch",
            )
            require(
                text(session, copied, b["revision"], 3)[0] == str(work / "source-a"),
                "copied cwd provenance mismatch",
            )
            copied_index = next(
                index
                for index in range(b["bindings"])
                if text(session, copied, b["revision"], 5, index)[0] == str(work / "visible")
            )
            require(
                text(session, copied, b["revision"], 2)[0] == str(work / "visible"),
                "import directory mismatch",
            )
            require(text(session, shared, a["revision"], 0)[0] == "/", "root mismatch")
            require(
                text(session, shared, a["revision"], 7)[0] == "",
                "directory binding has file component",
            )
            target, row = text(session, shared, a["revision"], 5)
            require(
                target == str(work / "visible") and row["binding_held"], "binding identity absent"
            )
            require(
                text(session, shared, a["revision"], 6)[0] == str(work / "source-a"),
                "initial source mismatch",
            )
            for bad in (
                f"namespace {shared} extra",
                f"namespace-read {shared} 18446744073709551616 0 0 0",
                f"namespace-read {shared} -1 0 0 0",
                f"namespace-read {shared} {a['revision']} 0 8 0",
                f"namespace-read {shared} {a['revision']} 10 5 0",
                f"namespace-read {shared} {a['revision']} 0 1 999999",
                f"namespace-read {shared} {a['revision']} 0 1 0 trailing",
            ):
                require(
                    not session.command(bad).get("ok"),
                    "malformed namespace command accepted: " + bad,
                )
            process.stdin.write("1")
            process.stdin.flush()
            line(process, "NSCHANGED")
            stale = session.command(
                f"namespace-read {shared} {a['revision']} 0 6 {len(str(work).encode())}"
            )
            if control:
                require(
                    stale.get("ok") and bytes.fromhex(stale["hex"]) == b"/source-b",
                    "revision omission did not expose actual new source bytes",
                )
                require(
                    stale["revision"] != a["revision"],
                    "revision omission did not change observed version",
                )
            else:
                require(
                    stale.get("error") == "namespace changed", "mixed-revision source was accepted"
                )
            current = session.command(f"namespace {shared}")
            require(
                int(current["revision"]) > int(a["revision"]), "shared revision did not advance"
            )
            require(
                text(session, shared, current["revision"], 6)[0] == str(work / "source-b"),
                "shared bind not observed",
            )
            require(
                text(session, copied, b["revision"], 6, copied_index)[0] == str(work / "source-a"),
                "copy followed peer mutation",
            )
            for pid in (shared, copied):
                require(
                    session.command(f"status {pid}")["retained"], "query released frozen source"
                )
                require(session.command(f"terminate {pid}").get("ok"), "termination refused")
            process.stdin.write("2")
            process.stdin.flush()
            line(process, "NSDONE")
            require(process.wait(timeout=10) == 0, "namespace debugger changed application exit")
        finally:
            session.close()
            if process.poll() is None:
                process.kill()
                process.wait()
            process.stdin.close()
            process.stdout.close()
    output = log.read_text()
    require("ThreadSanitizer:" not in output and "runtime error:" not in output, output)
    verify(output, 2)
    print(
        f"{'DETECTED mixed-revision real source' if control else 'PASS namespace debugger'} {mode} locked={locked}: shared mutation, isolated copy, stale refusal, UTF-8 pages"
    )


# EN/RU: Compile ordinary example; real path exceeds one text chunk and contains Unicode/quotes.
# Собрать обычный пример; настоящий путь длиннее chunk, содержит Unicode/quotes.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="namespace debugger ") as temporary:
        work = Path(temporary) / ('ю"' * 55)
        work.mkdir()
        for name in ("source-a", "source-b", "visible"):
            (work / name).mkdir()
        image = work / "main.dis"
        compile_image(
            compiler, include, root, root.parent / "examples/namespace-debug/main.b", image
        )
        for mode in ("--no-jit", "--jit"):
            for locked in (False, True):
                run(runtime, image, work, mode, locked)
        binary = relink(
            runtime, work, root / "vfs/test_observation_probe.c", 1, ["dvm_namespace_observe"]
        )
        for mode in ("--no-jit", "--jit"):
            run(binary, image, work, mode, False, True)


if __name__ == "__main__":
    main()
