# Copyright (c) 2026 bhgv
# EN: Test generated C-module ABI, native collectors and permanent Dis owners.
# RU: Проверка ABI сгенерированного C-модуля, сборщиков и закреплённых владельцев Dis.

from pathlib import Path
import re
import subprocess
import sys


# EN: Read key=value fields from exactly one statistics line named prefix in text.
# Inputs are borrowed; return a new dictionary, or raise for missing/duplicate data.
# RU: Прочитать key=value из единственной строки prefix в заимствованном text.
# Вернуть новый словарь; отсутствие или дублирование строки считается ошибкой.
def record(text, prefix):
    lines = [line for line in text.splitlines() if line.startswith(prefix + " ")]
    if len(lines) != 1:
        raise RuntimeError("missing or duplicate statistic: " + prefix)
    return dict(field.split("=", 1) for field in lines[0].split()[1:])


# EN: Verify borrowed statistics for mode after a real tutorial run. Require
# concurrent Fast collection, balanced retirement, no migrated owner or pending
# queue. Historical inline gc_runs is deliberately not a collector progress metric.
# RU: Проверить заимствованную статистику запуска в mode. Требовать конкурентный
# Fast GC, баланс освобождений, отсутствие миграций и незавершённых очередей.
# Старый счётчик inline gc_runs не является показателем работы этих сборщиков.
def check_statistics(text, mode):
    counters = re.search(r"instructions=(\d+) native=(\d+)", text)
    if not counters or int(counters[1]) == 0 or (int(counters[2]) > 0) != (mode == "jit"):
        raise RuntimeError("wrong native/interpreter dispatch: " + text)
    policy = record(text, "NATIVEGC_POLICY")
    if policy != {"exclusive": "0", "trace": "0", "stop_forbidden": "1"}:
        raise RuntimeError("unexpected GC execution policy: " + text)
    gc = record(text, "NATIVEGC")
    if gc["collector"] != "worker" or gc["joined"] != "1" or gc["failed"] != "0" or int(gc["fast_passes"]) == 0:
        raise RuntimeError("concurrent Fast GC evidence missing: " + text)
    rc = record(text, "NATIVERC")
    if rc["enqueued"] != rc["released"] or rc["pending"] != "0" or rc["bytes"] != "0":
        raise RuntimeError("native retirement is not balanced: " + text)
    dispatch = record(text, "NATIVEDISPATCH")
    queue = record(text, "NATIVEQUEUE")
    if dispatch["active"] != "0" or dispatch["migrated"] != "0" or queue["inflight"] != "0" or queue["queued"] != "0":
        raise RuntimeError("resident owner or queue did not retire: " + text)


# EN: argv supplies runtime, tutorial bytecode and jit/interpreter. Run both
# instruction budgets; every call checks real wrap/string/error/channel behavior.
# Child output is owned, calls time out and failures propagate with diagnostics.
# RU: argv задаёт runtime, байткод примера и jit/interpreter. Проверить два бюджета
# инструкций и реальное поведение wrap/строк/ошибок/каналов. Вывод принадлежит тесту;
# вызовы ограничены таймаутом, ошибки передаются с диагностикой.
def main():
    runtime, module = (Path(value).resolve() for value in sys.argv[1:3])
    mode = sys.argv[3]
    if mode not in ("jit", "interpreter"):
        raise ValueError("expected jit or interpreter")
    for quantum in (1, 2048):
        result = subprocess.run(
            [str(runtime), "--jit" if mode == "jit" else "--no-jit",
             "--stats", "--quantum", str(quantum), "--max-instructions", "1000000", str(module)],
            capture_output=True, text=True, timeout=30,
        )
        if result.returncode or "PASS native module: wrap=ok sum=42 strings=2048 caught=1 shutdown=ok" not in result.stdout:
            raise RuntimeError("native module tutorial failed: " + result.stdout + result.stderr)
        check_statistics(result.stderr, mode)
    print("PASS native C module: generated ABI, wrap, strings, caught error, channels, concurrent Fast GC " + mode)


if __name__ == "__main__":
    main()
