# Copyright (c) 2026 bhgv
# EN: Execute generated call instructions with a compiled caller and separate target.
# RU: Выполнить сгенерированный вызов со скомпилированным клиентом и отдельной целью.
from pathlib import Path
import subprocess
import sys
import tempfile

from relink_private import relink


# EN: Build independent fixtures and run a private interpreter/JIT regression.
# RU: Собрать независимые образы и запустить частную проверку интерпретатора и JIT.
def main():
    runtime, compiler, include = [Path(value).resolve() for value in sys.argv[1:]]
    fixtures = Path(__file__).resolve().parent / "fixtures" / "styx-generated-call"
    with tempfile.TemporaryDirectory(prefix="dis-styx-generated-call-") as temporary:
        work = Path(temporary)
        for name in ("client", "target"):
            subprocess.run([compiler, "-I", include, "-o", work / f"{name}.dis", fixtures / f"{name}.b"],
                           check=True, timeout=20)
        probe = relink(runtime, work, fixtures / "wrapper.c", 0, ["xec", "dvm_namespace_read_imports"])
        for mode in ("--no-jit", "--jit"):
            result = subprocess.run([probe, mode, "client.dis"], cwd=work, text=True,
                                    capture_output=True, timeout=30)
            text = result.stdout + result.stderr
            expected = ("STYX generated call installed", "PASS synthesized Dis call executed", "PASS loaded synthesized Dis module executed",
                        "PASS Dis application completed")
            if result.returncode or not all(marker in text for marker in expected):
                raise RuntimeError(f"{mode}: {text}")
            print(f"PASS generated Styx call with managed arguments {mode}")


if __name__ == "__main__":
    main()
