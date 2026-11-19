# Copyright (c) 2026 bhgv
# EN: Reproduce the old singleton publication race in a private native relink; leave production objects unchanged.
# RU: Воспроизвести прежнюю гонку публикации в отдельном бинарнике, не меняя production objects.
from pathlib import Path
import resource
import shlex
import signal
import subprocess
import sys
import tempfile


# EN: Disable core artifacts only in the intentional failing child, immediately before exec.
# RU: Отключить core-файлы только намеренно падающего дочернего процесса перед exec.
def no_core():
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))


# EN: Run the private compiler/linker command in its build directory; reject build errors, never count them as detection.
# RU: Выполнить частную компиляцию/линковку в каталоге сборки; ошибки сборки не считаются обнаружением гонки.
def build(command, directory):
    result = subprocess.run(command, cwd=directory, capture_output=True, text=True, timeout=60)
    if result.returncode:
        raise RuntimeError("device control build failed\n" + result.stdout + result.stderr)


# EN: Borrow the unit executable path from argv; replace only its device object with the old load/store algorithm.
# Require a precise semantic abort after actual concurrent factories; timeouts and sanitizer errors are failures.
# RU: По пути unit-бинарника из argv заменить только объект устройства прежним алгоритмом load/store.
# Требовать точный семантический отказ реальных конструкторов; таймауты и ошибки санитайзера не принимаются.
def main():
    binary = Path(sys.argv[1]).resolve()
    directory = binary.parent
    metadata = directory / "CMakeFiles" / (binary.name + ".dir")
    flags = {}
    for line in (metadata / "flags.make").read_text().splitlines():
        if " = " in line:
            key, value = line.split(" = ", 1)
            flags[key] = shlex.split(value)
    source = Path(__file__).with_name("sysevents_device.c")
    text = source.read_text()
    anchor = "    DVM_Provider *expected = NULL;\n"
    if text.count(anchor) != 1:
        raise RuntimeError("device publication control anchor changed")
    start = text.index(anchor)
    end = text.index("    return provider;", start)
    changed = text[:start] + "    __atomic_store_n(&device, provider, __ATOMIC_RELEASE);\n" + text[end:]
    with tempfile.TemporaryDirectory(prefix="sysevents-device-control-") as name:
        work = Path(name)
        replacement = work / "device.c"
        replacement.write_text(changed)
        obj = work / "device.o"
        link = shlex.split((metadata / "link.txt").read_text())
        build([link[0], *flags["C_DEFINES"], *flags["C_INCLUDES"], *flags["C_FLAGS"],
               "-I" + str(source.parent), "-c", str(replacement), "-o", str(obj)], directory)
        targets = [index for index, value in enumerate(link) if value.endswith("/vfs/sysevents_device.c.o")]
        if len(targets) != 1:
            raise RuntimeError("device object boundary changed")
        link[targets[0]] = str(obj)
        control = work / "device-control"
        link[link.index("-o") + 1] = str(control)
        build(link, directory)
        result = subprocess.run([control], cwd=work, capture_output=True, text=True, timeout=20, preexec_fn=no_core)
        output = result.stdout + result.stderr
        if result.returncode != -signal.SIGABRT or "sysevents device: multiple published device factories" not in output:
            raise RuntimeError("old publication race escaped semantic detection\n" + output)
        if "ERROR: AddressSanitizer" in output or "runtime error:" in output:
            raise RuntimeError("sanitizer failure is not semantic detection\n" + output)
    print("PASS old sysevents publication algorithm rejected by actual concurrent factory test")


if __name__ == "__main__":
    main()
