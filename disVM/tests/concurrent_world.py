# Copyright (c) 2026 bhgv
# [GC:] EN: Audit genuine overlap and exact rejection of both collector stop APIs.
# RU: Проверить настоящее пересечение и точный отказ обоих API остановки сборщика.
import subprocess
import sys


# [GC:] EN: Borrow binary path from CLI; own each process until exit. Check both
# polling modes, real ACTIVE overlap, and diagnostic status 70 for forbidden stops.
# Timeouts, signals and sanitizer output are failures.
# RU: Заимствовать путь binary из CLI; владеть процессами до выхода. Проверить
# оба polling, настоящее ACTIVE-пересечение и код 70 при запрещённых остановках.
# Таймауты, сигналы и сообщения санитайзера означают отказ.
def main():
    for locked in (0, 1):
        for mode in ("run", "stop", "idle"):
            result = subprocess.run(
                [sys.argv[1], mode, str(locked)], capture_output=True, text=True, timeout=10
            )
            output = result.stdout + result.stderr
            if "Sanitizer" in output or "runtime error:" in output:
                raise RuntimeError(output)
            expected = 0 if mode == "run" else 70
            marker = "PASS concurrent world" if mode == "run" else "reason=exclusive-stop-forbidden"
            if result.returncode != expected or marker not in output:
                raise RuntimeError(f"{mode} locked={locked} exit={result.returncode}\n{output}")
    print("PASS concurrent world: both polls, real overlap, both stop guards")


if __name__ == "__main__":
    main()
