# Copyright (c) 2026 bhgv
# EN: Real dedicated interpreter/JIT launch-binding acceptance, not a simulated UI/RPC test.
# RU: Настоящие dedicated interpreter/JIT проверки binding, не имитация UI/RPC.
from pathlib import Path
import subprocess
import sys
import tempfile
from test_owner_location import compile_image
from relink_private import relink


# EN/RU: Compile the actual guest and require explicit binding/lifetime evidence in both modes.
# Собрать настоящий guest, проверить binding/lifetime в обоих режимах VM.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="Unibus binding ") as name:
        work = Path(name)
        source = root / "fixtures/unibus-binding/main.b"
        compile_image(compiler, include, root, source, work / "main.dis")
        for variant in range(4):
            binary = relink(runtime, work, root / "test_unibus_binding.c", variant,
                            ["dvm_runtime_launch_module", "dvm_namespace_boot_shutdown"])
            for mode in ("--no-jit", "--jit"):
                result = subprocess.run(
                    [binary, "--native-spawn", mode, "--stats", work / "main.dis"],
                    cwd=work, capture_output=True, text=True, timeout=45,
                )
                output = result.stdout + result.stderr
                if result.returncode or "PASS real pre-init Unibus dispatch and isolation" not in output or \
                        "PASS pre-init Unibus native ownership drain" not in output:
                    raise RuntimeError(str(result.returncode) + ": " + output[-16000:])
                print("PASS Unibus variant=" + str(variant) + " " + mode, flush=True)
        for source, target in (("console.b", "console.dis"), ("console-main.b", "console-main.dis")):
            compile_image(compiler, include, root, root / "fixtures/unibus-binding" / source, work / target)
        for mode in ("--no-jit", "--jit"):
            result = subprocess.run(
                [runtime, "--native-spawn", mode, work / "console-main.dis"],
                cwd=work, input="console-fd\n", capture_output=True, text=True, timeout=45,
            )
            if result.returncode or "PASS console stdout without display" not in result.stdout or \
                    "PASS console stderr without display" not in result.stderr or \
                    (work / "console.txt").read_bytes() != b"data":
                raise RuntimeError("console FD regression: " + result.stdout + result.stderr)
            print("PASS nil-display console stdin/stdout/stderr/files " + mode, flush=True)


if __name__ == "__main__":
    main()
