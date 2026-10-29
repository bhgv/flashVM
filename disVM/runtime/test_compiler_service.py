"""Compare real Dis-native builds with CLI libraries and execute their artifacts."""

from pathlib import Path
import json
import os
import re
import subprocess
import sys
import tempfile


# Borrow argv/cwd/environment and return captured text after a bounded tool call.
# Raise on process, sanitizer or missing semantic evidence; own no child afterward.
def call(argv, cwd, environment):
    result = subprocess.run(
        list(map(str, argv)), cwd=cwd, env=environment, text=True, capture_output=True, timeout=45
    )
    if (
        result.returncode
        or "runtime error:" in result.stderr
        or "ERROR: AddressSanitizer" in result.stderr
    ):
        raise RuntimeError(str(argv) + "\n" + result.stdout + result.stderr)
    return result


# Explicit runtime/module/mode and matching CLI paths supply independent byte and
# execution oracles. Optional FlashVM runner enables actual linked SWF execution.
def main():
    runtime, module = [Path(value).resolve() for value in sys.argv[1:3]]
    mode = "--jit" if sys.argv[3] == "jit" else "--no-jit"
    source_cli, fir_cli, swl = [Path(value).resolve() for value in sys.argv[4:7]]
    flashvm = Path(sys.argv[7]).resolve() if len(sys.argv) > 7 else None
    with tempfile.TemporaryDirectory(prefix="compiler service ") as directory:
        work = Path(directory)
        environment = os.environ.copy()
        environment.update(TMPDIR=str(work), TMP=str(work), TEMP=str(work))
        prior = None
        for quantum in (1, 2048):
            result = call(
                [
                    runtime,
                    mode,
                    "--stats",
                    "--quantum",
                    quantum,
                    "--max-instructions",
                    1000000,
                    module,
                ],
                work,
                environment,
            )
            if (
                "PASS compiler service: native=24 workspace=ok spans=ok copies=ok shutdown=ok"
                not in result.stdout
            ):
                raise RuntimeError("missing service completion\n" + result.stdout + result.stderr)
            stats = re.search(r"instructions=(\d+) native=(\d+) gc_runs=(\d+)", result.stderr)
            if (
                not stats
                or int(stats[1]) == 0
                or (int(stats[2]) > 0) != (mode == "--jit")
                or (quantum == 1 and int(stats[3]) == 0)
                or "tables_live=0 keys_live=0" not in result.stderr
            ):
                raise RuntimeError("wrong service dispatch/GC/teardown\n" + result.stderr)
            snapshot = {
                path.name: path.read_bytes() for path in work.glob("compiler-*") if path.is_file()
            }
            if prior is not None and snapshot != prior:
                raise RuntimeError("GC/scheduler quantum changed artifact or debug bytes")
            prior = snapshot
        source = work / "input.b"
        raw, selected = work / "raw.fir", work / "selected.fir"
        expected, debug = work / "expected", work / "expected.debug"
        for language in range(1, 5):
            # Original source is copied by FIRDBG1, avoiding a duplicated source fixture.
            original = json.loads((work / f"compiler-{language}-0-1.debug").read_text())
            source.write_text(original["source"])
            call(
                [
                    source_cli,
                    ("as1", "as2", "as3", "limbo")[language - 1],
                    "--input",
                    source,
                    raw,
                    "--module",
                    "ide_build",
                    "-g",
                    debug,
                ],
                work,
                environment,
            )
            if json.loads(debug.read_text()) != original:
                raise RuntimeError("native and CLI source/debug records differ")
            for optimize in (0, 1):
                if optimize:
                    call([fir_cli, "opt", raw, selected], work, environment)
                else:
                    # print canonicalizes source-emitted text exactly as the service does.
                    call([fir_cli, "print", raw, selected], work, environment)
                for target in range(1, 4):
                    actual = work / f"compiler-{language}-{optimize}-{target}"
                    if target == 1:
                        call([fir_cli, "dis", selected, expected], work, environment)
                        executed = call(
                            [runtime, mode, "--quantum", 1, "--inspect-words", 0, 2, actual],
                            work,
                            environment,
                        )
                        if language == 4 and "DATA 0 42\nDATA 8 1\n" not in executed.stdout:
                            raise RuntimeError("native-compiled Limbo image did not execute 42")
                        if (
                            language != 4
                            and "DATA 0 4631107791820423168\nDATA 8 1\n" not in executed.stdout
                        ):
                            raise RuntimeError(
                                "native-compiled AS image did not execute binary64 42"
                            )
                    elif target == 2:
                        expected.write_bytes(selected.read_bytes())
                    else:
                        call([fir_cli, "object", selected, work / "object.swo"], work, environment)
                        call(
                            [swl, "link", expected, "--size", 640, 480, work / "object.swo"],
                            work,
                            environment,
                        )
                        if (
                            flashvm
                            and float(call([flashvm, actual], work, environment).stdout.strip())
                            != 42
                        ):
                            raise RuntimeError("native-compiled SWF did not execute 42")
                    if actual.read_bytes() != expected.read_bytes():
                        raise RuntimeError("native/CLI artifact bytes differ: " + actual.name)
                    if json.loads((work / (actual.name + ".debug")).read_text()) != original:
                        raise RuntimeError("optimization/target changed original debug provenance")
        encoded = (work / "compiler-error-source").read_bytes()
        line, first, size = map(int, (work / "compiler-error-location").read_text().split())
        if line != 2 or encoded[first : first + size] != b"missing":
            raise RuntimeError("native diagnostic lost original UTF-8 byte coordinates")
    print(
        "PASS compiler service: 24 exact CLI artifacts, original debug, native execution, workspace and "
        + mode
    )


if __name__ == "__main__":
    main()
