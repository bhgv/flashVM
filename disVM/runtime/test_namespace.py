# Copyright (c) 2026 bhgv
# EN: Real native guests exercise standard Sys namespace calls and host I/O.
# RU: Native-гости проверяют стандартные Sys namespace и настоящий host I/O.
from pathlib import Path
import subprocess
import sys
import tempfile

from test_owner_location import compile_image
from test_debug_mailbox import verify
from relink_private import relink


# EN/RU: Prepare real disjoint directories in borrowed temporary work; return created path.
# Подготовить реальные отдельные каталоги в temporary work; вернуть created path.
def prepare(work):
    for name, value in (("source-a", "A"), ("source-b", "B"), ("visible", "V")):
        directory = work / name
        directory.mkdir(exist_ok=True)
        (directory / "token").write_text(value)
    created = work / "source-a/created"
    created.unlink(missing_ok=True)
    return created


# EN: Relink only a test binary omitting guards. Require an actual sanitizer race
# at the omitted helper; compile failure/crash/timeout/normal exit is not evidence.
# RU: Перелинковать только test binary без guards. Требовать реальную TSan race
# в helper; compile failure/crash/timeout/normal exit не засчитываются.
def omission(runtime, root, work, image):
    binary = relink(runtime, work, root / "vfs/test_raw_result.c", 0, ["dvm_sys_scalar_result"])
    for mode in ("--no-jit", "--jit"):
        prepare(work)
        result = subprocess.run(
            [str(binary), mode, str(image)], cwd=work, capture_output=True, text=True, timeout=45
        )
        output = result.stdout + result.stderr
        if (
            not result.returncode
            or "ThreadSanitizer: data race" not in output
            or "__wrap_dvm_sys_scalar_result" not in output
        ):
            raise RuntimeError("namespace scalar omission not detected\n" + output[-16000:])
        print(f"DETECTED namespace raw result race {mode}")


# EN: Compile the public example, prepare disjoint real resources and run both
# dispatch/poll choices. Require complete native retirement and exact host writes.
# RU: Собрать public example, подготовить разные реальные ресурсы и проверить
# оба dispatch/poll. Требовать полный native retirement и точные host-записи.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    marker = "NAMESPACE shared=B copied=A descriptor=V create=C root=A restored=V"
    with tempfile.TemporaryDirectory(prefix="guest namespace ") as temporary:
        work = Path(temporary)
        image = work / "main.dis"
        compile_image(compiler, include, root, root.parent / "examples/namespace/main.b", image)
        if sys.argv[4:] == ["--controls-only"]:
            omission(runtime, root, work, image)
            return
        for mode in ("--no-jit", "--jit"):
            for locked in (False, True):
                created = prepare(work)
                result = subprocess.run(
                    [
                        str(runtime),
                        mode,
                        "--stats",
                        "--quantum",
                        "7",
                        *(["--gc-locked-poll"] if locked else []),
                        str(image),
                    ],
                    cwd=work,
                    capture_output=True,
                    text=True,
                    timeout=45,
                )
                output = result.stdout + result.stderr
                if result.returncode or marker not in output or "ThreadSanitizer:" in output:
                    raise RuntimeError("namespace acceptance failed\n" + output[-16000:])
                verify(output, 7)
                if created.read_text() != "C" or (work / "visible/created").exists():
                    raise RuntimeError("namespace create escaped its binding")
                print(f"PASS namespace {mode} locked={locked}")


if __name__ == "__main__":
    main()
