# Copyright (c) 2026 bhgv
# EN: Run constructor/channel payload checks only on permanent native owners.
# RU: Выполнить проверки конструкторов/каналов только на постоянных native-владельцах.
from pathlib import Path
import subprocess
import sys
import tempfile

from resident_evidence import evidence
from test_record_new import CONCURRENT_GUEST  # Data-only Limbo oracle; no legacy probe is executed.


# EN: Borrow matching CLI build products. Compile the unchanged forty-owner
# payload and require exact results, native identity and no-stop policy in both
# dispatch/poll modes. Own files/processes until exit; failures raise.
# RU: Заимствовать согласованные CLI-продукты. Собрать неизменный payload сорока
# владельцев; требовать точные результаты, native-идентичность и запрет stop с обоими
# dispatch/poll. Владеть файлами/процессами до выхода; при сбое вызвать исключение.
def main():
    runtime, compiler, include = (Path(arg).resolve() for arg in sys.argv[1:])
    with tempfile.TemporaryDirectory(prefix="native records ") as temporary:
        work = Path(temporary)
        source = work / "records.b"
        image = work / "records.dis"
        source.write_text(CONCURRENT_GUEST)
        subprocess.run(
            [str(compiler), "-I", str(include), "-o", str(image), str(source)],
            check=True,
            capture_output=True,
            text=True,
        )
        for mode in ("--jit", "--no-jit"):
            for poll in ((), ("--gc-locked-poll",)):
                result = subprocess.run(
                    [str(runtime), mode, "--stats", *poll, str(image)],
                    cwd=work,
                    capture_output=True,
                    text=True,
                    timeout=60,
                )
                output = result.stdout + result.stderr
                if (
                    result.returncode
                    or output.count("PASS forty allocation workers") != 1
                    or "ThreadSanitizer:" in output
                ):
                    raise RuntimeError(f"native constructors {mode} {poll}\n{output}")
                evidence(output, children=40)
    print(
        "PASS native constructors: forty permanent owners, records, arrays, channels, C/JIT and both polls"
    )


if __name__ == "__main__":
    main()
