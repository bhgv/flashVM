# Copyright (c) 2026 bhgv
# EN: Owner location geometry, imported-module lifetime and public example acceptance.
# RU: Owner location geometry, lifetime imported module и приёмка пользовательского примера.
from pathlib import Path
import subprocess
import sys
import tempfile

from relink_private import relink
from resident_evidence import evidence
from test_debug_mailbox import verify


# EN/RU: Compile matched source/include paths; compiler errors never count as runtime evidence.
# Собрать matched source/include paths; compiler errors не считаются runtime evidence.
def compile_image(compiler, include, root, source, output, extra=()):
    command = [str(compiler), "-I", str(include), "-I", str(root.parent / "module")]
    for directory in extra:
        command += ["-I", str(directory)]
    result = subprocess.run(
        command + ["-o", str(output), str(source)], capture_output=True, text=True
    )
    if result.returncode:
        raise RuntimeError("owner location compile failed\n" + result.stdout + result.stderr)


# EN: Run complete native VM and require normal exit, exact marker and full ownership drain.
# RU: Запустить native VM; требовать normal exit, точный marker и полный ownership drain.
def run(binary, image, mode, locked, marker, children, quantum=1000):
    result = subprocess.run(
        [
            str(binary),
            "--stats",
            "--quantum",
            str(quantum),
            mode,
            *(["--gc-locked-poll"] if locked else []),
            str(image),
        ],
        capture_output=True,
        text=True,
        timeout=90,
    )
    output = result.stdout + result.stderr
    if (
        result.returncode
        or "ThreadSanitizer:" in output
        or "runtime error:" in output
        or marker not in output
    ):
        raise RuntimeError("owner location acceptance failed\n" + output[-12000:])
    verify(output, children)
    return output


# EN/RU: Exercise real imported callers, frozen tickets, reusable Limbo reader and example.
# Проверить imported callers, frozen tickets, reusable Limbo reader и пример.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    fixture = root / "fixtures/owner-location"
    with tempfile.TemporaryDirectory(prefix="owner location ") as temporary:
        work = Path(temporary)
        compile_image(
            compiler,
            include,
            root,
            root.parent / "debug/residentdebug.b",
            work / "residentdebug.dis",
        )
        compile_image(
            compiler, include, root, fixture / "provider.b", work / "provider-λ.dis", [fixture]
        )
        image = work / "main.dis"
        example = work / "example.dis"
        compile_image(compiler, include, root, fixture / "main.b", image, [fixture])
        compile_image(
            compiler, include, root, root.parent / "examples/resident-debug/main.b", example
        )
        fault = work / "fault"
        fault.mkdir()
        library = (root.parent / "debug/residentdebug.b").read_text()
        needle = "    bytes := array[length] of byte;"
        if library.count(needle) != 1:
            raise RuntimeError("reader failure injection boundary changed")
        altered = fault / "residentdebug.b"
        altered.write_text(
            library.replace(needle, needle + '\n    raise "location reader injected failure";')
        )
        compile_image(compiler, include, root, altered, fault / "residentdebug.dis")
        fault_image = fault / "main.dis"
        compile_image(compiler, include, root, fixture / "reader-failure.b", fault_image)
        symbols = [
            "dvm_diagnostic_location_capture",
            "dvm_resident_location_publish",
            "dvm_module_registry_release",
        ]
        binary = relink(runtime, work, root / "test_owner_location_probe.c", 0, symbols)
        for mode in ("--no-jit", "--jit"):
            for locked in (False, True):
                output = run(
                    binary,
                    image,
                    mode,
                    locked,
                    "PASS owner location: imported Unicode labels next PC frozen ticket later boundary library ownership joined",
                    2,
                )
                if (
                    "NATIVEDEBUGLOCATION owner=1 tls=1 geometry=1 utf8=1" not in output
                    or "NATIVEDEBUGLOCATION module_finalized=1" not in output
                ):
                    raise RuntimeError("owner/TLS geometry probe did not execute")
                run(runtime, example, mode, locked, "PASS resident debug example", 1)
                run(
                    binary,
                    fault_image,
                    mode,
                    locked,
                    "PASS owner location reader: managed error rethrow pending cleanup full quota native work preserved joined",
                    2,
                )
                print("PASS owner location " + mode + " locked=" + str(locked), flush=True)
            run(
                binary,
                image,
                mode,
                False,
                "PASS owner location: imported Unicode labels next PC frozen ticket later boundary library ownership joined",
                2,
                quantum=1,
            )
            print("PASS owner location " + mode + " quantum=1", flush=True)

        for control in (1, 2):
            broken = relink(runtime, work, root / "test_owner_location_probe.c", control, symbols)
            output = run(
                broken,
                image,
                "--no-jit",
                False,
                "DETECTED owner location missing or wrong copied module labels",
                2,
            )
            if "PASS owner location:" in output:
                raise RuntimeError("owner location omission unexpectedly accepted")
            print("PASS owner location omission " + str(control), flush=True)


if __name__ == "__main__":
    main()
