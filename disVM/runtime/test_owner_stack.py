# Copyright (c) 2026 bhgv
# EN: Native owner call-chain capture, imported recursion, cleanup and public example.
# RU: Native owner call-chain capture, imported recursion, cleanup и public example.
from pathlib import Path
import sys
import tempfile

from relink_private import relink
from test_owner_location import compile_image, run


# EN/RU: Compile independent fixtures; require normal semantic results and ownership drain.
# Собрать independent fixtures; требовать normal semantic results и ownership drain.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    fixture = root / "fixtures/owner-stack"
    with tempfile.TemporaryDirectory(prefix="owner stack ") as temporary:
        work = Path(temporary)
        compile_image(
            compiler,
            include,
            root,
            root.parent / "debug/residentstack.b",
            work / "residentstack.dis",
        )
        compile_image(
            compiler, include, root, fixture / "provider.b", work / "stack-λ.dis", [fixture]
        )
        image = work / "main.dis"
        compile_image(compiler, include, root, fixture / "main.b", image, [fixture])
        example = work / "example.dis"
        compile_image(
            compiler, include, root, root.parent / "examples/resident-stack/main.b", example
        )
        fault = work / "fault"
        fault.mkdir()
        library = (root.parent / "debug/residentstack.b").read_text()
        needle = "    bytes := array[length] of byte;"
        if library.count(needle) != 1:
            raise RuntimeError("stack reader injection boundary changed")
        altered = fault / "residentstack.b"
        altered.write_text(
            library.replace(needle, needle + '\n    raise "stack reader injected failure";')
        )
        compile_image(compiler, include, root, altered, fault / "residentstack.dis")
        fault_image = fault / "main.dis"
        compile_image(compiler, include, root, fixture / "reader-failure.b", fault_image)
        symbols = [
            "dvm_diagnostic_stack_capture",
            "dvm_resident_stack_publish",
            "dvm_module_registry_release",
            "extend",
        ]
        binary = relink(runtime, work, root / "test_owner_stack_probe.c", 0, symbols)
        marker = "PASS owner stack: imports recursion growth truncation frozen caller frames ownership bounds joined"
        for mode in ("--no-jit", "--jit"):
            for locked in (False, True):
                output = run(binary, image, mode, locked, marker, 4)
                if (
                    "NATIVEDEBUGSTACK owner=1 tls=1 geometry=1 bounded=1" not in output
                    or "NATIVEDEBUGSTACK module_finalized=1" not in output
                    or "NATIVEDEBUGSTACK stack_growth=1" not in output
                ):
                    raise RuntimeError("stack owner or provider lifetime probe did not execute")
                run(runtime, example, mode, locked, "PASS resident stack example", 1)
                run(
                    binary,
                    fault_image,
                    mode,
                    locked,
                    "PASS owner stack reader: managed error rethrow pending cleanup full quota native work preserved joined",
                    2,
                )
                print("PASS owner stack " + mode + " locked=" + str(locked), flush=True)
            run(binary, image, mode, False, marker, 4, quantum=1)
            print("PASS owner stack " + mode + " quantum=1", flush=True)
        for control in (1, 2):
            broken = relink(runtime, work, root / "test_owner_stack_probe.c", control, symbols)
            output = run(
                broken,
                image,
                "--no-jit",
                False,
                "DETECTED owner stack missing or wrong copied caller chain",
                4,
            )
            if marker in output:
                raise RuntimeError("owner stack omission unexpectedly accepted")
            print("PASS owner stack omission " + str(control), flush=True)


if __name__ == "__main__":
    main()
