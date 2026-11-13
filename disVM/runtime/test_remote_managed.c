/* Copyright (c) 2026 bhgv
 * EN: Refuse one existing-root retirement transfer in a private diagnostic link.
 * RU: Отказать одной передаче существующего root в частной diagnostic-сборке.
 */
#include <stdio.h>

static unsigned int attempts; /* Native atomic count; no managed pointer is inspected. */


/* [GC:] EN/RU: Borrow value only for the real transfer after the injected refusal.
 * Заимствовать value только для настоящей передачи после внедрённого отказа. */
int __real_dvm_runtime_rc_transfer_root(void *value);


/* [GC:] EN: CONTROL=1 refuses exactly one transfer unchanged; otherwise execute
 * production code. Never remove a root, touch heap memory or hide a failure.
 * RU: CONTROL=1 отказывает ровно одной передаче без изменений; иначе выполняет
 * production code. Не снимать roots, не читать heap и не скрывать failures. */
int __wrap_dvm_runtime_rc_transfer_root(void *value)
{
    if (CONTROL == 1 && __atomic_fetch_add(&attempts, 1, __ATOMIC_RELAXED) == 0)
    {
        fputs("REMOTE_ROOT_PRESSURE injected=1\n", stderr);
        return 0;
    }
    return __real_dvm_runtime_rc_transfer_root(value);
}
