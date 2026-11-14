/* Copyright (c) 2026 bhgv
 * EN: Separate GC participant for read-only frozen-continuation inspection.
 * RU: Отдельный GC participant для read-only frozen-continuation inspection.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "internal.h"
#include "runtime_gc.h"
#include "disvm/native_process.h"
#include "disvm/native_owner.h"
#include "thread_port.h"
#include <stdlib.h>

static __thread DVM_NativeContext *inspector; /* Transport's own context, never a guest executor. */


/* [GC:] EN/RU: Enter helper admission with no process/turn; source gate pins inspected roots.
 * Войти helper admission без process/turn; source gate удерживает исследуемые roots. */
int dvm_debug_inspector_enter(void)
{
    if (!inspector)
    {
        inspector = dvm_native_process_create(NULL);
        if (!inspector)
        {
            return 0;
        }
        if (!dvm_native_context_bind(inspector))
        {
            dvm_native_context_destroy(inspector);
            inspector = NULL;
            return 0;
        }
        up->env = &up->defenv;
        up->env->errstr = up->env->errbuf1;
        up->env->syserrstr = up->env->errbuf0;
        if (!dvm_runtime_gc_attach())
        {
            dvm_native_context_unbind(inspector);
            dvm_native_context_destroy(inspector);
            inspector = NULL;
            return 0;
        }
        return 1;
    }
    return dvm_native_owner_enter(dvm_native_owner_current());
}


/* [GC:] EN/RU: Drop every borrowed managed value before rootless helper leave.
 * Отбросить все borrowed managed values до rootless helper leave. */
void dvm_debug_inspector_leave(void)
{
    if (!dvm_native_owner_leave(dvm_native_owner_current()))
    {
        abort();
    }
}


/* [GC:] EN/RU: Detach and destroy the inspector before the runtime joins collectors.
 * Отключить и удалить inspector до runtime collector joins. */
void dvm_debug_inspector_stop(void)
{
    if (inspector)
    {
        while (!dvm_runtime_gc_detach())
        {
            fvm_port_thread_sleep_us(50);
        }
        if (!dvm_native_context_unbind(inspector) || !dvm_native_context_destroy(inspector))
        {
            abort();
        }
        inspector = NULL;
    }
}
