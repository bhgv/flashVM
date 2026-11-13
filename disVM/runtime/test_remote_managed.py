# Copyright (c) 2026 bhgv
# EN: Real managed export lifetime and failure controls in native C/JIT DisVM.
# RU: Реальный managed export lifetime и контроли отказов в native C/JIT DisVM.
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from relink_private import relink
from resident_evidence import evidence


# EN: Run matching runtime/image under explicit native C/JIT and poll selection.
# Own child through exit; output is copied, any compiler/runtime error fails.
# RU: Выполнить matching runtime/image в native C/JIT с выбранным poll.
# Владеть child до exit; output копируется, любая ошибка даёт отказ.
def execute(runtime, image, mode, locked):
    result = subprocess.run(
        [
            str(runtime),
            "--remote-owner-probe",
            "--stats",
            mode,
            *(["--gc-locked-poll"] if locked else []),
            str(image),
        ],
        cwd=image.parent,
        capture_output=True,
        text=True,
        timeout=60,
    )
    return result.returncode, result.stdout + result.stderr


# EN: Require actual managed roots, exact joined residents and stop-free collectors.
# Pressure variants must prove one refused transfer and later complete cleanup.
# RU: Проверить managed roots, joined residents и stop-free collectors.
# Pressure variants обязаны доказать отказ передачи и последующую очистку.
def verify(code, output, pressure):
    if code or "ThreadSanitizer:" in output or "runtime error:" in output:
        raise RuntimeError(output)
    evidence(output, children=1)
    required = (
        "PASS remote managed roots: 8 native peers, aliases, close, concurrent GC, rollback, exact Type baseline",
        f"REMOTEROOTS pressure={pressure} pending=0 owner_thread=exact",
        "PASS DisVM channel proxy",
    )
    if any(output.count(marker) != 1 for marker in required):
        raise RuntimeError("missing exact managed root proof\n" + output)
    if not re.search(
        r"NATIVERC enqueued=(\d+) released=\1 pending=0 saturated=0 bytes=0 byte_limited=0", output
    ):
        raise RuntimeError("managed export retirement did not drain\n" + output)
    if pressure and output.count("REMOTE_ROOT_PRESSURE injected=1") != 1:
        raise RuntimeError("pressure refusal not observed\n" + output)


# EN: Make one exact private omission, rejecting source drift before compilation.
# Root/ref/rollback omissions must fail a pre-access lifetime assertion, not crash.
# RU: Создать точный private omission, отказать при source drift до compilation.
# Пропуски root/ref/rollback должны поймать lifetime check до unsafe access.
def omission(root, work, index):
    text = (root / "remote_managed.c").read_text()
    anchors = (
        "if (dvm_refcount_retain_u(&heap->ref) != DVM_REFCOUNT_LIVE)",
        "if (!dvm_runtime_root_add(heap))",
        "if (!dvm_runtime_root_remove_if(heap, drop_added_reference, &heap->ref))",
    )
    anchor = anchors[index]
    if text.count(anchor) != 1:
        raise RuntimeError("remote root control anchor changed")
    output = work / f"omission-{index}.c"
    output.write_text(text.replace(anchor, "if (0)"))
    return output


# EN: CLI selects matching products; build real Limbo and private controls.
# Positive-only is portable native acceptance. All files/children remain owned
# until verified; production objects and source are never rewritten.
# RU: CLI задаёт matching products; собрать реальный Limbo и private controls.
# Positive-only — portable native acceptance. Файлы/children живут до проверки;
# production objects/source не переписываются.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    positive = sys.argv[4:] == ["--positive-only"]
    if sys.argv[4:] and not positive:
        raise RuntimeError("unsupported remote root test arguments")
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="remote managed roots ") as temporary:
        work = Path(temporary)
        image = work / "probe.dis"
        subprocess.run(
            [
                str(compiler),
                "-I",
                str(include),
                "-o",
                str(image),
                str(root.parents[1] / "examples/disvm/channel-proxy.b"),
            ],
            check=True,
            capture_output=True,
            text=True,
        )
        for mode in ("--no-jit", "--jit"):
            for locked in (False, True):
                verify(*execute(runtime, image, mode, locked), pressure=0)
        pressured = relink(
            runtime, work, root / "test_remote_managed.c", 1, ["dvm_runtime_rc_transfer_root"]
        )
        for mode in ("--no-jit", "--jit"):
            for locked in (False, True):
                verify(*execute(pressured, image, mode, locked), pressure=1)
        if not positive:
            for index in range(3):
                binary = relink(
                    runtime,
                    work,
                    root / "test_remote_managed.c",
                    0,
                    ["dvm_runtime_rc_transfer_root"],
                    remote_managed_source=omission(root, work, index),
                )
                code, output = execute(binary, image, "--no-jit", False)
                marker = (
                    "failed closed publish exact rollback"
                    if index == 2
                    else "independent target root and strong reference"
                )
                if code != 10 or f"FAIL remote managed roots: {marker}" not in output:
                    raise RuntimeError("ownership omission not detected\n" + output)
    print(
        "PASS remote managed exports: real roots, 8 native peers, C/JIT, both polls, pressure recovery"
    )
    if not positive:
        print("PASS root/ref/rollback omissions refused before unsafe access")


if __name__ == "__main__":
    main()
