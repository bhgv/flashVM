# Copyright (c) 2026 bhgv
# EN: Execute the menu's permanent-owner example and inspect actual runtime evidence.
# RU: Исполнить пример закреплённых владельцев из меню и проверить статистику runtime.

from pathlib import Path
import re
import subprocess
import sys


# EN/RU: Borrow text/prefix; return exactly one native key=value statistics row.
# Заимствовать text/prefix; вернуть единственную строку native key=value.
def row(text, prefix):
    lines = [line for line in text.splitlines() if line.startswith(prefix + " ")]
    if len(lines) != 1:
        raise RuntimeError("missing statistics: " + prefix)
    return dict(item.split("=", 1) for item in lines[0].split()[1:])


# EN: argv gives runtime, bytecode and jit/interpreter. Check real child owners,
# copied replies and joined cleanup; child calls are bounded, no GUI is required.
# RU: argv задаёт runtime, байткод и jit/interpreter. Проверить владельцев дочерних
# процессов, ответы и join; вызов ограничен таймаутом, GUI не требуется.
def main():
    runtime, module = (Path(value).resolve() for value in sys.argv[1:3])
    mode = sys.argv[3]
    if mode not in ("jit", "interpreter"):
        raise ValueError("expected jit or interpreter")
    result = subprocess.run(
        [str(runtime), "--native-spawn", "--jit" if mode == "jit" else "--no-jit", "--stats", str(module)],
        text=True, capture_output=True, timeout=45,
    )
    if result.returncode or "PASS resident owners: children=4 sums=ok private-state=ok channels=ok" not in result.stdout:
        raise RuntimeError(result.stdout + result.stderr)
    workers = row(result.stderr, "NATIVEWORKERS")
    dispatch = row(result.stderr, "NATIVEDISPATCH")
    queue = row(result.stderr, "NATIVEQUEUE")
    if workers["threads"] != "5" or workers["joined"] != "1" or workers["failed"] != "0":
        raise RuntimeError("missing permanent child owners: " + result.stderr)
    if dispatch["migrated"] != "0" or dispatch["active"] != "0" or queue["inflight"] != "0":
        raise RuntimeError("owner migration or incomplete retirement: " + result.stderr)
    counter = re.search(r"instructions=(\d+) native=(\d+)", result.stderr)
    if not counter or int(counter[1]) == 0 or (int(counter[2]) > 0) != (mode == "jit"):
        raise RuntimeError("wrong dispatch: " + result.stderr)
    print("PASS menu resident owners: four native children, copied replies, no migration " + mode)


if __name__ == "__main__":
    main()
