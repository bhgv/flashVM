# Copyright (c) 2026 bhgv
# EN: Live allocator accounting API under native churn, shared scalar writes and refusals.
# RU: Live allocator accounting API при native churn, shared scalar writes и refusals.
from pathlib import Path
import sys
import tempfile

from relink_private import relink
from test_owner_location import compile_image, run


# EN/RU: Compile matched public library/example and private native/accounting controls.
# Собрать matched public library/example и private native/accounting controls.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    fixture = root / "fixtures/memory-statistics/main.b"
    with tempfile.TemporaryDirectory(prefix="memory statistics ") as temporary:
        work = Path(temporary)
        compile_image(
            compiler, include, root, root.parent / "debug/memoryview.b", work / "memoryview.dis"
        )
        image = work / "main.dis"
        compile_image(compiler, include, root, fixture, image)
        refusal_source = work / "refusal.b"
        source = fixture.read_text()
        if source.count("Refusal: con 0;") != 1:
            raise RuntimeError("memory statistics refusal fixture drift")
        refusal_source.write_text(source.replace("Refusal: con 0;", "Refusal: con 1;"))
        refused = work / "refusal.dis"
        compile_image(compiler, include, root, refusal_source, refused)
        example = work / "example.dis"
        compile_image(
            compiler, include, root, root.parent / "examples/memory-statistics/main.b", example
        )
        symbols = ["dvm_heap_pool_snapshot", "dvm_sys_scalar_result"]
        binary = relink(runtime, work, root / "test_memory_statistics_probe.c", 0, symbols)
        unavailable = relink(runtime, work, root / "test_memory_statistics_probe.c", 3, symbols)
        marker = "PASS memory statistics: coherent partition traffic shared result library ordinary work joined"
        for mode in ("--no-jit", "--jit"):
            for locked in (False, True):
                output = run(binary, image, mode, locked, marker, 5)
                if (
                    "NATIVEMEMORYUNIT owners=8 joined=1 oracle=1 saturated=1 zeroed=1" not in output
                    or "NATIVEMEMORY scalarbytes=96 owner=1" not in output
                ):
                    raise RuntimeError("actual pool oracle or wide scalar boundary did not run")
                run(runtime, example, mode, locked, "PASS memory statistics example", 1)
                run(
                    unavailable,
                    refused,
                    mode,
                    locked,
                    "PASS memory statistics refusal: zero output nil library ordinary work joined",
                    5,
                )
                print("PASS memory statistics " + mode + " locked=" + str(locked), flush=True)
            run(binary, image, mode, False, marker, 5, quantum=1)
            print("PASS memory statistics " + mode + " quantum=1", flush=True)
        for control in (1, 2):
            broken = relink(
                runtime, work, root / "test_memory_statistics_probe.c", control, symbols
            )
            output = run(
                broken,
                image,
                "--no-jit",
                False,
                "DETECTED memory statistics inconsistent copied accounting",
                5,
            )
            if marker in output:
                raise RuntimeError("memory statistics omission unexpectedly accepted")
            print("PASS memory statistics copied-output omission " + str(control), flush=True)


if __name__ == "__main__":
    main()
