/* Copyright (c) 2026 bhgv
 * [GC:] EN: Copy current owner's immutable launch data into independently owned managed results.
 * RU: Копировать immutable launch data своего owner в независимые managed results.
 */
#include "../cap_internal.h"
#include "runqueue.h"
#include "context.h"
#include "launchcontext_decl.h"
#include "launchcontext_table.h"


/* EN/RU: Borrow native context pinned by ACTIVE current process; no PID query, I/O or allocation.
 * Заимствовать native context своего ACTIVE process; без PID-query, I/O и allocation.
 */
static DVM_LaunchContext *current(void)
{
    Prog *process = currun();
    return process ? ((DVM_RuntimeProcess *)process)->launch_context : NULL;
}


/* [GC:] EN/RU: Root one freshly allocated owned value and publish through guarded result replacement.
 * value is a fresh owned object/H; destination is the borrowed mapped result slot, result is void.
 * No native release; source remains rooted through allocation/commit, exceptions retire the fresh owner.
 * Укоренить fresh owned value и выдать guarded заменой result; без native release,
 * value — fresh owned object/H, destination — borrowed mapped result, без возвращаемого значения.
 * Source укоренён до commit, exceptions освобождают fresh owner.
 */
static void publish(void *value, void *destination)
{
    volatile int rooted = 0; /* Only successful ptradd may be undone during an exception. */
    if (waserror())
    {
        if (value != H)
        {
            if (rooted)
            {
                ptrdel(D2H(value));
            }
            destroy(value);
        }
        nexterror();
    }
    if (value != H)
    {
        ptradd(D2H(value));
        rooted = 1;
        Setmark(D2H(value));
    }
    poperror();
    dvm_cap_publish(value, destination);
}


/* [GC:] EN/RU: Capture borrowed generated frame, return copied logical label/nil through its mapped result.
 * No I/O/native release; label conveys no filesystem/provider authority.
 * Захватить borrowed generated frame, выдать copied logical label/nil в mapped result;
 * без I/O/native release, label не передаёт filesystem/provider authority.
 */
void Launchcontext_name(void *frame)
{
    F_Launchcontext_name call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Launchcontext_name, ret), sizeof(*call.ret));
    DVM_LaunchContext *context = current(); /* Owner native pin survives every managed allocation. */
    String *value = context ? c2string(context->module, strlen(context->module)) : H;
    publish(value, call.ret);
}


/* [GC:] EN/RU: Capture borrowed generated frame, publish fresh payload copy/nil for empty/absent data.
 * No I/O/native release; repeated calls never alias native storage.
 * Захватить borrowed generated frame, выдать fresh payload copy/nil в mapped result;
 * без I/O/native release, вызовы не alias native storage.
 */
void Launchcontext_arguments(void *frame)
{
    F_Launchcontext_arguments call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Launchcontext_arguments, ret), sizeof(*call.ret));
    DVM_LaunchContext *context = current(); /* Bounded native data, never reinterpreted as managed pointers. */
    Array *value = context && context->argument_bytes ? mem2array(context->arguments, context->argument_bytes) : H;
    publish(value, call.ret);
}


/* EN/RU: Register generated matched builtin before any process publication, no per-application adapter.
 * Зарегистрировать generated matched builtin до publication process, без per-application adapter.
 */
void dvm_launch_context_initialize(void)
{
    builtinmod("$Launchcontext", Launchcontextmodtab, Launchcontextmodlen);
}
