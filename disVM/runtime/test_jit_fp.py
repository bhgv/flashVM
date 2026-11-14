# Copyright (c) 2026 bhgv
# EN: Kernel FP canaries, real-operation fault handling and source-local fenv restoration.
# RU: Kernel FP canaries, обработка real-operation fault и source-local fenv restoration.
from pathlib import Path
import json
import re
import resource
import signal
from native_arch_suite import elf_backend
import sys
import tempfile
from relink_private import relink
from test_owner_location import compile_image
from test_jit_signal import execute, require


# EN/RU: Compare complete named base FP bits with independently declared RX canaries.
# Сравнить complete named base FP bits с независимо объявленными RX canaries.
def verify(fp, divide=False):
    require(fp.get("ok") and fp.get("coverage") == "base", str(fp))
    registers = fp["registers"]
    if fp["architecture"] == "x64":
        require(len(registers) == 16 and fp["bits"] == 128, str(fp))
        require(registers["xmm0"] == "88776655443322111122334455667788", str(fp))
        require(registers["xmm15"] == "00000000000000008877665544332211", str(fp))
        require(fp["x87"]["st0"] == "3fff8000000000000000", str(fp))
        require(
            int(fp["control"], 16) & 0xC00 == 0xC00
            and int(fp["extra"], 16) & 0x603F == (0x6004 if divide else 0x6000),
            str(fp),
        )
        require(int(fp["tag"], 16) != 0, str(fp))
    elif fp["architecture"] == "arm64":
        require(len(registers) == 32 and fp["bits"] == 128, str(fp))
        require(registers["v0"] == "88776655443322111122334455667788", str(fp))
        require(registers["v31"] == "11223344556677888877665544332211", str(fp))
        require(int(fp["control"], 16) == 0xC00000 and int(fp["status"], 16) == 0, str(fp))
    else:
        require(
            fp["architecture"] == "riscv64" and len(registers) == 32 and fp["bits"] == 64, str(fp)
        )
        require(
            registers["f0"] == "1122334455667788" and registers["f31"] == "8877665544332211",
            str(fp),
        )
        require(int(fp["control"], 16) == 0x20 and int(fp["status"], 16) == 0x20, str(fp))


# EN: Compile the actual REAL guest. Safe omission controls preserve ordinary
# source completion; inspect copied FP and real pre-handler fenv evidence first.
# RU: Собрать настоящий REAL guest. Safe omissions сохраняют normal source
# completion; сначала проверить copied FP и actual pre-handler fenv evidence.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="native FP ") as temporary:
        work = Path(temporary)
        image = work / "main.dis"
        compile_image(compiler, include, root, root.parent / "examples/jit-fp/main.b", image)
        symbols = [
            "dvm_runtime_fast",
            "dvm_bigtable_call_ready",
            "dvm_jit_native_error",
            "fesetenv",
            "dvm_jit_fault_copy",
            "dvm_debug_fault",
            "dvm_debug_native_gate",
            "dvm_gc_fast_shared",
            "dvm_close_worker_stop",
        ]
        for control in ((0, 1, 2, 3) if elf_backend(runtime) == "amd64" else (0, 1, 2)):
            binary = relink(runtime, work, root / "test_jit_fp_probe.c", control, symbols)
            for locked in (False, True) if control in (0, 3) else (False,):
                for debug in (False, True) if control in (0, 3) else (True,):

                    # EN: session/pid select copied source FP; cpu is prevalidated GPR metadata.
                    # Return an omission label only after independent pre-handler evidence.
                    # RU: session/pid выбирают copied FP; cpu — проверенные GPR metadata.
                    # Вернуть omission label только после независимой pre-handler проверки.
                    def inspect(session, pid, cpu):
                        log = (work / f"signal-10-{debug}-{locked}.log").read_text()
                        require("JITFPENV restored=" + ("0" if control == 2 else "1") in log, log)
                        copied = session.command(f"native-fp {pid}")
                        if control == 1:
                            require(copied.get("error") == "no valid base FP context", str(copied))
                            return (
                                "DETECTED missing copied FP context with normal source retirement"
                            )
                        verify(copied, control == 3)
                        return (
                            "DETECTED missing FP environment restoration with normal source retirement"
                            if control == 2
                            else None
                        )

                    output = execute(
                        binary,
                        image,
                        work,
                        10,
                        debug,
                        locked,
                        inspect,
                        signal.SIGFPE if control == 3 else None,
                    )
                    require("JITFPENV restored=" + ("0" if control == 2 else "1") in output, output)
                    selected = re.search(r"JITFPOPCODE selected=(\d+)", output)
                    location = re.search(r"JITFAULT [^\n]*opcode=(\d+)", output)
                    require(
                        selected and location and selected[1] == location[1],
                        "wrong original REAL opcode: " + output,
                    )
                    rows = re.findall(r"JITFP pid=\d+ (\{[^\n]+\})", output)
                    require(len(rows) == 1, output)
                    verify(json.loads(rows[0]), control == 3)


if __name__ == "__main__":
    main()
