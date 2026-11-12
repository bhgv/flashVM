# Copyright (c) 2026 bhgv
"""EN: Verify joined native residents and their permanent process assignments.
RU: Проверка завершённых нативных потоков и неизменных назначений процессов.
"""

import re
from native_gc_evidence import require_concurrent_gc


def evidence(output, children):
    """EN: Validate combined runtime output for exactly children spawned residents.
    Require matched thread lifetimes, fixed ownership and drained native/GC work.
    Raise RuntimeError with diagnostics on failure. output is borrowed text.
    RU: Проверить вывод для children дочерних потоков, постоянство владельцев,
    завершение потоков, native и GC. При нарушении вызвать RuntimeError.
    """
    require_concurrent_gc(output)
    retirement_evidence(output, children)
    starts = re.findall(r"NATIVEGUEST start pid=(\d+) thread=(\d+)", output)
    retired = re.findall(r"NATIVEGUEST retired pid=(\d+) thread=(\d+)", output)
    dispatch = re.search(
        r"NATIVEDISPATCH claims=(\d+) finishes=(\d+) active=0 migrated=0\b", output
    )
    leases = re.search(r"NATIVEQUEUE queued=0 leases=(\d+) returns=(\d+) inflight=0", output)
    workers = re.search(r"NATIVEWORKERS threads=(\d+) joined=1 failed=0", output)
    if (
        len(starts) != children
        or len({pid for pid, _ in starts}) != children
        or sorted(starts) != sorted(retired)
        or not dispatch
        or int(dispatch[1]) == 0
        or dispatch[1] != dispatch[2]
        or not leases
        or int(leases[1]) == 0
        or leases[1] != leases[2]
        or not workers
        or int(workers[1]) < 2
        or not re.search(r"NATIVEGC .*joined=1 failed=0", output)
        or "PASS Dis application completed" not in output
    ):
        raise RuntimeError("resident ownership or retirement failed:\n" + output[-6000:])


# EN: Reconcile borrowed output against exactly children retired resident slots.
# Each joined slot contributes once to service totals, whether reaped during
# execution or joined at shutdown. Raise on missing/duplicate IDs or lost sums.
# RU: Сверить заимствованный output с children завершёнными слотами процессов.
# Каждый join учитывается один раз при текущей сборке либо остановке сервиса.
# При пропущенном/повторном ID или потере сумм вызвать исключение.
def retirement_evidence(output, children):
    rows = re.findall(
        r"NATIVEWORKERS_(?:REAP|THREAD) id=(\d+) joined=1 "
        r"turns=(\d+) releases=(\d+) reentries=(\d+)",
        output,
    )
    totals = re.search(
        r"NATIVEWORKERS threads=\d+ joined=1 failed=0 controller_turns=\d+ "
        r"worker_turns=(\d+) worker_releases=(\d+) worker_reentries=(\d+)",
        output,
    )
    sums = tuple(sum(int(row[column]) for row in rows) for column in range(1, 4))
    if (
        len(rows) != children
        or len({row[0] for row in rows}) != children
        or not totals
        or sums != tuple(map(int, totals.groups()))
    ):
        raise RuntimeError(
            "joined resident accounting lost or duplicated a slot:\n" + output[-6000:]
        )
