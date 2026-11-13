# Copyright (c) 2026 bhgv
# EN: Real emergency strings retire before continuously admitted peers leave.
# RU: Настоящие резервные строки освобождаются до выхода допущенных соседей.
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from relink_private import relink
from resident_evidence import evidence


# EN: Compile the native relay against unchanged runtime objects, then run C/JIT,
# both poll backends and a disabled-shared-path control. CLI supplies exact build
# paths. Own temporary binaries/children; reject compile errors and timeout results.
# RU: Собрать эстафету с настоящими объектами runtime, проверить C/JIT, оба режима
# опроса и контроль с отключённым shared-путём. CLI задаёт пути сборки. Владеть
# файлами/процессами; ошибки компиляции и таймаут не считать успехом контроля.
# EN: --positive-only keeps the four real release profiles without broken variants.
# RU: --positive-only оставляет четыре настоящих release-профиля без сломанных вариантов.
def main():
    positive_only = "--positive-only" in sys.argv[1:]
    runtime, compiler, include = (
        Path(value).resolve() for value in sys.argv[1:] if value != "--positive-only"
    )
    root = Path(__file__).parent
    with tempfile.TemporaryDirectory(prefix="shared exception retirement ") as temporary:
        work = Path(temporary)
        image = work / "exceptions.dis"
        subprocess.run(
            [
                str(compiler),
                "-I",
                str(include),
                "-I",
                str(root.parent / "module"),
                "-o",
                str(image),
                str(root / "fixtures/resident-exception-retirement/main.b"),
            ],
            check=True,
            capture_output=True,
            text=True,
        )
        for control in (0,) if positive_only else (0, 1, 2):
            replacement = None
            if control == 2:
                source = root / "gc/fast/exception_retirement.c"
                original = source.read_text()
                gate = "if (!dvm_world_epoch_elapsed(world, entry->epoch))"
                if original.count(gate) != 1:
                    raise RuntimeError("emergency reader-epoch control anchor changed")
                # Keep relative includes resolving in the original module directory.
                replacement = work / "exception_retirement.c"
                changed = original.replace(gate, "if (0)")
                for include_name in (
                    "../common/exception_queue.h",
                    "../../exception_host.h",
                    "../../root_registry.h",
                ):
                    changed = changed.replace(
                        '"' + include_name + '"', '"' + str(source.parent / include_name) + '"'
                    )
                replacement.write_text(changed)
            binary = relink(
                runtime,
                work,
                Path(__file__).with_suffix(".c"),
                int(control == 1),
                [
                    "dvm_runtime_guest_sleep",
                    "Sys_werrstr",
                    "poolfree",
                    "dvm_runtime_exception_retire_shared",
                ],
                exception_retirement_source=replacement,
            )
            profiles = (
                [("--no-jit", False)]
                if control
                else [(mode, locked) for mode in ("--no-jit", "--jit") for locked in (False, True)]
            )
            for mode, locked in profiles:
                result = subprocess.run(
                    [
                        str(binary),
                        "--native-spawn",
                        "--gc-no-cycles",
                        "--stats",
                        mode,
                        *(["--gc-locked-poll"] if locked else []),
                        str(image),
                    ],
                    cwd=work,
                    capture_output=True,
                    text=True,
                    timeout=25,
                )
                text = result.stdout + result.stderr
                if control:
                    marker = (
                        "emergency retirement still requires heap quiescence"
                        if control == 1
                        else "emergency string freed before reader epoch elapsed"
                    )
                    if result.returncode != 12 or marker not in text:
                        raise RuntimeError(f"control {control} was not detected\n" + text[-12000:])
                    continue
                if (
                    result.returncode
                    or "PASS shared exception retirement final=1 external=1 admitted=2 reader_wait=1"
                    not in text
                ):
                    raise RuntimeError(f"{mode} locked={locked}\n{text[-12000:]}")
                evidence(text, children=2)
                counters = re.search(
                    r"NATIVEEXCEPTION shared=(\d+) with_peer=(\d+) retained=(\d+) pending=0", text
                )
                if not counters or int(counters[1]) < 2 or int(counters[2]) < 2:
                    raise RuntimeError(
                        "real shared exception retirement was not counted\n" + text[-12000:]
                    )
                if "PASS resident exception retirement peers=2" not in text:
                    raise RuntimeError("resident continuations did not finish\n" + text[-12000:])
    print(
        "PASS shared emergency retirement: final and externally rooted strings, "
        "held reader epoch, admitted peers, C/JIT, both polls"
        + (
            "; positive profiles only"
            if positive_only
            else "; disabled-path and early-free controls"
        )
    )


if __name__ == "__main__":
    main()
