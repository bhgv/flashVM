# Copyright (c) 2026 bhgv
# EN: Actual dedicated startup metadata, constructor rollback and display-free console acceptance.
# RU: Настоящие dedicated startup metadata, constructor rollback и display-free console acceptance.
from pathlib import Path
import subprocess
import sys
import tempfile

# EN/RU: Explicitly locate repository-owned source-matched compile/relink helpers.
# Явно найти repository-owned source-matched compile/relink helpers.
sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from test_owner_location import compile_image
from relink_private import relink


# EN/RU: Run one actual guest; require semantic markers and all native ownership checks, not only an exit code.
# Выполнить настоящий guest; требовать semantic markers и native ownership checks, не только exit code.
def execute(binary, image, mode, work, markers):
    result = subprocess.run([binary, "--native-spawn", mode, image], cwd=work,
                            input="console-input\n", capture_output=True, text=True, timeout=45)
    output = result.stdout + result.stderr
    if result.returncode or any(marker not in output for marker in markers):
        raise RuntimeError(f"launch context {mode}\n" + output)


# EN/RU: Compile portable sources and verify ordinary/accepted binary, empty and maximum-size launches in both engines.
# Собрать portable sources и проверить ordinary/accepted binary, empty и maximum-size launch в обоих движках.
def main():
    runtime, compiler, include = [Path(value).resolve() for value in sys.argv[1:4]]
    root = Path(__file__).resolve().parents[2]
    fixtures = root / "fixtures/launch-context"
    with tempfile.TemporaryDirectory(prefix="launch-context-") as name:
        work = Path(name)
        compile_image(compiler, include, root, fixtures / "absent.b", work / "absent.dis")
        for mode in ("--no-jit", "--jit"):
            execute(runtime, work / "absent.dis", mode, work, ["PASS absent launch context"])
            print(f"PASS absent launch context {mode}", flush=True)
        for variant, length in enumerate((257, 0, 65536)):
            source = work / "main.b"
            source.write_text((fixtures / "main.b").read_text().replace("ARGUMENT_BYTES", str(length)))
            compile_image(compiler, include, root, source, work / "main.dis")
            binary = relink(runtime, work, root / "tests/launch_context/launch.c", variant,
                            ["dvm_runtime_launch_module", "dvm_namespace_boot_shutdown",
                             "dvm_launch_context_create", "malloc", "free"])
            for mode in ("--no-jit", "--jit"):
                execute(binary, work / "main.dis", mode, work,
                        ["PASS immutable launch context and child copies",
                         "PASS launch context console stdout", "PASS launch context console stderr",
                         "PASS launch context native snapshot lifetime"])
                print(f"PASS launch context length={length} {mode}", flush=True)


if __name__ == "__main__":
    main()
