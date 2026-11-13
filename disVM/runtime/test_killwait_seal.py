# Copyright (c) 2026 bhgv
# EN: Force the actual live-sample/sealed-child killwait admission window.
# RU: Принудительно совместить живой снимок и sealed-child в допуске killwait.
from pathlib import Path
import subprocess
import sys
import tempfile
from relink_private import relink
from resident_evidence import evidence

SYMBOLS = [  # Private observations; every production implementation still executes.
    "dvm_runtime_prepare_killwait",
    "dvm_runtime_killwait_result",
    "dvm_runqueue_hold_count",
    "dvm_runqueue_begin_retire",
]


# EN: Restore one obsolete second admission load in a private main composition.
# root/work are borrowed paths; return owned source path. Drift raises before
# compilation; no production object or source is changed.
# RU: Вернуть второе старое чтение допуска в частной композиции main.
# root/work заимствованы; вернуть путь личного исходника. Дрейф даёт отказ
# до сборки; production-исходники и объекты не изменяются.
def old_admission(root, work):
    fragment = (root / "host_runtime/process_control.inc").read_text()
    anchor = "    int retiring = dvm_runqueue_hold_count(child) < 0;\n"
    if fragment.count(anchor) != 1:
        raise RuntimeError("killwait seal control anchor changed")
    old = work / "old-process-control.inc"
    old.write_text(
        fragment.replace(
            anchor,
            anchor + """    if (!retiring && !resident_can_exit(child))
    {
        fvm_port_sync_unlock(channel_queue_mutex);
        return 0;
    }
""",
            1,
        )
    )
    main = (root / "main.c").read_text()
    include = '#include "host_runtime/process_control.inc"'
    if main.count(include) != 1:
        raise RuntimeError("killwait seal main composition changed")
    result = work / "old-main.c"
    result.write_text(main.replace(include, f'#include "{old}"', 1))
    return result


# EN: CLI borrows matching VM/compiler/include; own temporary code and children.
# All positive C/JIT/poll runs require real parking and result Retired. Restore
# the old admission in controls, requiring exact NotWaiting/exit12 evidence.
# --positive-only excludes intentional failures, never checks or timeouts.
# RU: CLI заимствует VM/компилятор/include; владеть файлами и дочерними процессами.
# Все C/JIT/poll обязаны дать реальное ожидание и Retired. Контроли возвращают
# старый допуск и требуют точный NotWaiting/код12. --positive-only исключает
# намеренные отказы, а не проверки или таймауты.
def main():
    positive = "--positive-only" in sys.argv[1:]
    runtime, compiler, include = (Path(v).resolve() for v in sys.argv[1:] if v != "--positive-only")
    root = Path(__file__).parent
    with tempfile.TemporaryDirectory(prefix="killwait lifetime seal ") as directory:
        work = Path(directory)
        image = work / "seal.dis"
        subprocess.run(
            [
                str(compiler),
                "-I",
                str(include),
                "-I",
                str(root.parent / "module"),
                "-o",
                str(image),
                str(root / "fixtures/killwait-seal/main.b"),
            ],
            check=True,
            capture_output=True,
            text=True,
        )
        for control in (0,) if positive else (0, 1):
            binary = relink(
                runtime,
                work,
                root / "test_killwait_seal.c",
                control,
                SYMBOLS,
                runtime_main_source=old_admission(root, work) if control else None,
            )
            for mode in ("--no-jit", "--jit"):
                for locked in (False, True):
                    result = subprocess.run(
                        [
                            str(binary),
                            "--native-spawn",
                            mode,
                            "--stats",
                            *(["--gc-locked-poll"] if locked else []),
                            str(image),
                        ],
                        cwd=work,
                        capture_output=True,
                        text=True,
                        timeout=60,
                    )
                    output = result.stdout + result.stderr
                    if "Sanitizer" in output or "runtime error:" in output:
                        raise RuntimeError("killwait seal sanitizer failure\n" + output[-12000:])
                    marker = f"KILLWAIT_SEAL parked={0 if control else 1} result={0 if control else 1} sampled_live=1 sealed=1"
                    if control:
                        valid = (
                            result.returncode == 12
                            and marker in output
                            and "FAIL killwait seal: registered child returned NotWaiting" in output
                        )
                    else:
                        valid = (
                            result.returncode == 0
                            and marker in output
                            and "PASS killwait admits child sealing during preflight" in output
                        )
                        if valid:
                            evidence(output, children=1)
                    if not valid:
                        raise RuntimeError(
                            f"control={control} {mode} locked={locked}\n{output[-12000:]}"
                        )
    print(
        "PASS real child seals between parent lifetime load and wait admission: C/JIT, both polls"
        + ("; positive only" if positive else "; actual old admission returned NotWaiting controls")
    )


if __name__ == "__main__":
    main()
