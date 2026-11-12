# Copyright (c) 2026 bhgv
# EN: Actual root replacement with geometry-independent native observations.
# RU: Настоящая замена корней с native-наблюдениями без предположений о хеше.
"""Actual preallocated root replacement, native/locked ports and semantic controls."""

from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile


def main():
    """Compile private actual-source copies; no native key enters a VM collector.

    --positive-only retains both real backends/stress, omitting deliberate defects.
    CFLAGS may instrument actual C. Only exact semantic10 satisfies a control.
    RU: Владеть частными native-сборками и процессами до выхода. CLI positive-only
    оставляет оба backend без намеренных ошибок; CFLAGS инструментирует исходники.
    Контроль требует точной ошибки 10, не таймаута, сигнала или ошибки компиляции.
    """
    runtime = Path(__file__).resolve().parent
    ports = runtime.parents[1] / "flashVM/ports"
    if not ports.exists():
        ports = runtime.parent / "ports"
    original = (runtime / "root_batch.c").read_text()
    begin = original.index("int dvm_runtime_root_batch_replace(")
    end = original.index("\n}\n", begin) + 3
    body = original[begin:end]
    relink = (
        "    for (position = 0; position < new_count; position++)\n    {\n        RootEntry *node"
    )
    if body.count(relink) != 1:
        raise RuntimeError("replacement relink anchor drift")
    controls = {
        "interval": (
            relink,
            "    fvm_port_sync_unlock(batch->mutex);\n    fvm_port_sync_lock(batch->mutex);\n"
            + relink,
            "one registry interval",
        ),
        "withdraw": (
            "            unlink_entry(batch, node);",
            "            (void)node;",
            "old roots withdrawn before reuse",
        ),
        "publish": ("        *bucket = node;", "        (void)bucket;", "new multiset"),
        "validate": (
            "        if (!keys[position])",
            "        if (0 && !keys[position])",
            "invalid vector unchanged",
        ),
        "indices": (
            "    batch->count = new_count;",
            "    batch->count = 0;",
            "new index numbering",
        ),
        "allocate": (
            "    fvm_port_sync_lock(batch->mutex);",
            "    void *volatile attempted = malloc(1);\n    free(attempted);\n    fvm_port_sync_lock(batch->mutex);",
            "replace allocation attempt",
        ),
    }
    with tempfile.TemporaryDirectory(prefix="native root replacement ") as directory:
        work = Path(directory)
        for backend in ("native", "locked"):
            for variant in ([None] if "--positive-only" in sys.argv else [None, *controls]):
                code = body
                if variant:
                    before, after, marker = controls[variant]
                    if code.count(before) != 1:
                        raise RuntimeError("replacement control drift: " + variant)
                    code = code.replace(before, after)
                withdraw = "    for (position = batch->count; position > 0; position--)"
                if code.count(withdraw) != 1:
                    raise RuntimeError("old-key capture anchor drift")
                code = code.replace(
                    withdraw,
                    "    dvm_test_before_withdraw(batch->entries, batch->count);\n" + withdraw,
                )
                code = code.replace(relink, "    dvm_test_before_relink(batch);\n" + relink)
                source = original[:begin] + code + original[end:]
                source = (
                    '#include "root_batch.h"\n#include "root_registry_private.h"\nextern void dvm_test_before_withdraw(RootEntry *, size_t);\nextern void dvm_test_before_relink(DVM_RootBatch *);\n'
                    + source
                )
                (work / "batch.c").write_text(source)
                command = shlex.split(os.environ.get("CC", "cc")) + shlex.split(
                    os.environ.get("CFLAGS", "-O2 -g")
                )
                command += [
                    "-std=c99",
                    "-Wall",
                    "-Wextra",
                    "-Werror",
                    "-pthread",
                    "-I" + str(runtime),
                    "-I" + str(ports),
                ]
                if backend == "locked":
                    command += ["-DDVM_ROOT_FORCE_LOCKED=1"]
                command += [
                    str(runtime / "test_root_replace.c"),
                    str(work / "batch.c"),
                    str(runtime / "root_registry.c"),
                    str(ports / "linux/thread.c"),
                    str(ports / "linux/channel_sync.c"),
                    "-Wl,--wrap=malloc",
                    "-Wl,--wrap=calloc",
                    "-Wl,--wrap=free",
                    "-Wl,--wrap=fvm_port_sync_lock",
                    "-Wl,--wrap=fvm_port_sync_unlock",
                    "-o",
                    str(work / "test"),
                ]
                built = subprocess.run(command, capture_output=True, text=True)
                if built.returncode:
                    raise RuntimeError("replacement compile failed: " + built.stdout + built.stderr)
                args = [str(work / "test")] + (["--serial-only"] if variant else [])
                run = subprocess.run(args, capture_output=True, text=True, timeout=60)
                output = run.stdout + run.stderr
                if "WARNING: ThreadSanitizer:" in output or "runtime error:" in output:
                    raise RuntimeError(output)
                if variant:
                    if run.returncode != 10 or "FAIL root replace: " + marker not in output:
                        raise RuntimeError("replacement control missed " + variant + ": " + output)
                elif run.returncode or "PASS root replacement:" not in output:
                    raise RuntimeError("replacement positive failed: " + output)
                else:
                    print(backend + ": " + output.strip())
    print(
        "PASS native root replacement"
        + ("; positives only" if "--positive-only" in sys.argv else "; twelve exact-stage controls")
    )


if __name__ == "__main__":
    main()
