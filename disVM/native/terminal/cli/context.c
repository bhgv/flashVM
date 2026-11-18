/* Copyright (c) 2026 bhgv
 * EN: Standalone native executable has no guest owner/VM cleanup ledger; native handles retain explicit lifecycle.
 * RU: У standalone native-процесса нет guest owner/VM cleanup-ledger; native-handles освобождаются явно.
 */
#include "disvm/native_cleanup.h"
#include <stddef.h>


/* EN/RU: This executable runs no VM; no managed execution context exists on any relay thread.
 * Этот executable не исполняет VM; ни у одного relay-потока нет managed execution context.
 */
DVM_NativeContext *dvm_native_context_current(void)
{
    return NULL;
}


/* EN/RU: Refuse VM-ledger registration unchanged; callers without a VM use their documented explicit native cleanup.
 * Отказать VM-регистрации без изменений; callers без VM используют документированный явный native-cleanup.
 */
int dvm_native_cleanup_link(DVM_NativeCleanup *record, int (*cleanup)(void *), void *argument)
{
    (void)record;
    (void)cleanup;
    (void)argument;
    return 0;
}


/* EN/RU: No VM-ledger entry can belong to this standalone executable; preserve caller state,0.
 * В этом standalone executable нет VM-ledger entries; сохранить state caller,0.
 */
int dvm_native_cleanup_unlink(DVM_NativeCleanup *record)
{
    (void)record;
    return 0;
}
