# Copyright (c) 2026 bhgv
# [GC:] EN: Strict native collection evidence shared by application regressions.
# RU: Подтверждения строгой native-сборки для проверок настоящих приложений.
import re


# [GC:] EN: Borrow complete --stats output and require joined collectors with
# zero retained tracing, snapshots, epochs and fallback passes. Queue draining
# and workload-specific reclamation remain the calling test's responsibility.
# Return normally on success; raise on missing or contradictory evidence.
# RU: Заимствовать полный --stats и требовать join сборщиков при нулевых
# резервных обходах, снимках, эпохах и fallback-проходах. Обработку очередей и
# сборку конкретной нагрузки проверяет вызывающий тест. Успех — возврат, иначе
# исключение при отсутствующих либо противоречивых подтверждениях.
def require_concurrent_gc(output):
    gc = re.search(
        r"NATIVEGC collector=worker passes=(\d+) snapshots=(\d+) "
        r"reclaimed=(\d+) joined=1 failed=0",
        output,
    )
    counters = re.search(r"instructions=\d+ native=\d+ gc_runs=(\d+) gc_epochs=(\d+)", output)
    if (
        "NATIVEGC_POLICY exclusive=0 trace=0 stop_forbidden=1" not in output
        or not gc
        or any(map(int, gc.groups()))
        or not counters
        or any(map(int, counters.groups()))
        or not re.search(r"NATIVERCPASSES idle=0\b", output)
        or not re.search(r"NATIVERCFALLBACK moved=0 pending=0\b", output)
    ):
        raise RuntimeError(
            "strict native collection evidence missing or retained path entered:\n" + output[-8000:]
        )
