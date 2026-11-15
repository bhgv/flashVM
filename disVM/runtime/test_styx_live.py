# Copyright (c) 2026 bhgv
# EN: Exercise matched existing Styx server handlers and codec, without a network-transport claim.
# RU: Проверить existing Styx handlers/codec, без заявления о network transport.
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from test_owner_location import compile_image


# EN/RU: Compile one native guest, retain matched library products, run C/JIT and both GC settings.
# Собрать guest, удержать matched libraries, выполнить C/JIT и оба GC settings.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="styx live ") as directory:
        work = Path(directory)
        for name in ("styx", "styxservers"):
            shutil.copyfile(runtime.parent / (name + ".dis"), work / (name + ".dis"))
        compile_image(compiler, include, root, root / "fixtures/styx-live/main.b", work / "main.dis")
        for mode in ("--no-jit", "--jit"):
            for locked in (False, True):
                result = subprocess.run([str(runtime), mode, "--stats", *(["--gc-locked-poll"] if locked else []),
                                         str(work / "main.dis")], cwd=work, text=True, capture_output=True, timeout=60)
                output = result.stdout + result.stderr
                if result.returncode or "PASS Styx live roots" not in output or "ThreadSanitizer:" in output:
                    raise RuntimeError(output[-16000:])
                print(f"PASS Styx live {mode} locked={locked}", flush=True)


if __name__ == "__main__":
    main()
