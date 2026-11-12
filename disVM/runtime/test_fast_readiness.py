# Copyright (c) 2026 bhgv
# EN: Actual native readiness race and private omission of the admission hint.
# RU: Настоящая гонка готовности и частное удаление проверки допуска.
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile


# EN: Run native binary, requiring exact success or omission failure. binary/cwd
# are borrowed paths; never accept compiler errors, signals or sanitizer reports.
# RU: Запустить binary из cwd, требуя точного успеха или ошибки контроля.
# Пути заимствованы; ошибки сборки, сигналы и санитайзер не считаются проверкой.
def run(binary, cwd, omission=False):
    result = subprocess.run([str(binary)], cwd=cwd, capture_output=True, text=True, timeout=20)
    text = result.stdout + result.stderr
    if "WARNING: ThreadSanitizer:" in text or "runtime error:" in text:
        raise RuntimeError("Fast readiness sanitizer failure\n" + text)
    code = 12 if omission else 0
    marker = (
        "active producer must keep Fast on shared retry"
        if omission
        else "PASS Fast readiness: actual publication after empty sample"
    )
    if result.returncode != code or marker not in text:
        raise RuntimeError("Fast readiness semantic check missed\n" + text)


# EN: Own a private host dispatcher relink beside the actual CMake binary from
# CLI. Preserve matching compiler/flags/headers; production objects stay intact.
# --positive-only omits the intentional violation for repeated sanitizer checks.
# RU: Владеть личной перелинковкой host-диспетчера для CLI бинарника CMake.
# Сохранить компилятор/флаги/заголовки; исходная сборка не меняется.
# --positive-only исключает намеренное нарушение для повторов санитайзера.
def main():
    positive_only = "--positive-only" in sys.argv[1:]
    binary = Path(next(value for value in sys.argv[1:] if value != "--positive-only")).resolve()
    build = binary.parent
    run(binary, build)
    if not positive_only:
        metadata = build / "CMakeFiles/disvm_fast_readiness_test.dir"
        flags = {}
        for line in (metadata / "flags.make").read_text().splitlines():
            if " = " in line:
                key, value = line.split(" = ", 1)
                flags[key] = shlex.split(value)
        link = [
            value
            for value in shlex.split((metadata / "link.txt").read_text())
            if not value.startswith("-Wl,--dependency-file=")
        ]
        source = Path(__file__).parent.parent / "host/gc_worker_pass.c"
        text = source.read_text()
        gate = (
            "        if (worker->kind == DVM_GC_FAST && dvm_world_has_active_guest(workers->config.world))\n"
            "        {\n            return DVM_GC_MORE;\n        }"
        )
        if text.count(gate) != 1:
            raise RuntimeError("Fast guest-admission hint anchor changed")
        with tempfile.TemporaryDirectory(prefix="Fast readiness omission ") as temporary:
            work = Path(temporary)
            copied = work / source.name
            copied.write_text(text.replace(gate, ""))
            obj = work / "dispatcher.o"
            subprocess.run(
                [
                    link[0],
                    *flags["C_DEFINES"],
                    *flags["C_INCLUDES"],
                    *flags["C_FLAGS"],
                    "-I" + str(source.parent),
                    "-c",
                    str(copied),
                    "-o",
                    str(obj),
                ],
                check=True,
                capture_output=True,
                text=True,
            )
            output = work / "readiness-omission"
            link[link.index("-o") + 1] = str(output)
            link.insert(1, str(obj))
            subprocess.run(link, cwd=build, check=True, capture_output=True, text=True)
            run(output, build, omission=True)
    print(
        "PASS Fast readiness: native publication, both polls, admitted retry and actual retained cleanup"
        + ("" if positive_only else "; semantic admission-hint omission control")
    )


if __name__ == "__main__":
    main()
