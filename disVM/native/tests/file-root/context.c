/* Copyright (c) 2026 bhgv
 * EN: Standalone native SDK fixture has no VM context; managed cleanup registration is forbidden.
 * RU: Standalone native SDK fixture не имеет VM context; managed cleanup registration запрещён.
 */
#include "disvm/native_cleanup.h"
#include <stddef.h>


/* EN/RU: Native network fixture owns no guest Prog/REG; return no VM execution context.
 * Native network fixture не владеет guest Prog/REG; вернуть отсутствие VM context. */
DVM_NativeContext *dvm_native_context_current(void)
{
    return NULL;
}


/* EN/RU: Refuse cleanup registration without a VM context; no state is adopted or modified.
 * Отказать cleanup registration без VM context; state не принимается/не меняется. */
int dvm_native_cleanup_link(DVM_NativeCleanup *record, int (*cleanup)(void *), void *argument)
{
    (void)record;
    (void)cleanup;
    (void)argument;
    return 0;
}


/* EN/RU: No VM cleanup records can exist in this standalone native fixture.
 * В standalone native fixture не может быть VM cleanup records. */
int dvm_native_cleanup_unlink(DVM_NativeCleanup *record)
{
    (void)record;
    return 0;
}
