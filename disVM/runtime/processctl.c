/* Copyright (c) 2026 bhgv
 * EN: Captured native scalar operations with guarded shared result publication.
 * RU: Captured native scalar operations с guarded shared result publication.
 */


/* Hosted scalar process control through generated NC64 frames. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "raise.h"
#include "sys_capture.h"
#include "managed_workers.h"
#include "processctlmod.h"
#include <limits.h>
#include <stddef.h>


#include "diagnostics/processctl_calls.inc"
#include "diagnostics/location_calls.inc"
#include "diagnostics/stack_calls.inc"


extern int dvm_runtime_prepare_cancelwait(Prog *caller, int pid);
extern int dvm_runtime_cancel_child_wait(Prog *caller, int pid);
extern int dvm_runtime_kill_child(Prog *caller, int pid);
extern int dvm_runtime_prepare_killwait(Prog *caller, int pid);
extern int dvm_runtime_killwait_result(Prog *caller, int pid);


/* EN: Stage current Prog identity, then publish through result cell guards.
 * frame is a rooted generated call; BUSY retries only publication, not the PID query.
 * RU: Сохранить identity текущего Prog и опубликовать под result cell guards.
 * frame — rooted generated call; BUSY повторяет только publication, не запрос PID. */
void Processctl_pid(void *frame)
{
    F_Processctl_pid call;
    WORD result;

    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Processctl_pid, ret), sizeof(*call.ret));
    result = currun()->pid;
    dvm_sys_scalar_result(&result, call.ret, sizeof(result));
}


/* EN: Borrow the rooted generated frame; stage one native metadata card before
 * guarded scalar publication. No guest pointer, register or heap graph is read.
 * Failed/empty reads return zero fields. The builtin's resident keeps service alive.
 * RU: Borrow rooted generated frame; сохранить native metadata card до guarded
 * scalar publication. Guest pointer, register и heap graph не читаются. Failed/empty
 * read возвращает zero fields. Вызывающий resident удерживает service lifetime. */
void Processctl_resident(void *frame)
{
    F_Processctl_resident call;

    struct
    {
        WORD status; /* Card, end or refusal; independent from payload. */
        LONG id;     /* Copied service-lifetime ID in signed-big range. */
        WORD pid;    /* Permanent guest PID, not a pointer. */
        WORD done;   /* Published completion after TLS teardown. */
    } result = {-1, 0, 0, 0};

    DVM_ResidentCard card;
    unsigned int count;
    int more;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Processctl_resident, ret), sizeof(*call.ret));
    if (sizeof(result) != sizeof(*call.ret))
    {
        error(exBounds);
    }
    if (call.after >= 0 && dvm_resident_observe((unsigned long)call.after, &card, 1, &count, &more))
    {
        result.status = 0;
        if (count && card.id <= (unsigned long)LLONG_MAX)
        {
            result.status = 1;
            result.id = (LONG)card.id;
            result.pid = card.pid;
            result.done = card.done;
        }
        else if (count)
        {
            result.status = -1;
        }
    }
    dvm_sys_scalar_result(&result, call.ret, sizeof(result));
}


/* EN: Borrow the rooted generated frame; stage one native metadata/phase card before
 * guarded scalar publication. No guest pointer, register or heap graph is read.
 * Failed/empty reads return zero fields. The builtin's resident keeps service alive.
 * RU: Borrow rooted generated frame; сохранить native metadata card до guarded
 * scalar publication. Guest pointer, register и heap graph не читаются. Failed/empty
 * read возвращает zero fields. Вызывающий resident удерживает service lifetime. */
void Processctl_residentstate(void *frame)
{
    F_Processctl_residentstate call;

    struct
    {
        WORD status; /* Card, end or refusal; independent from payload. */
        LONG id;     /* Copied service-lifetime ID in signed-big range. */
        WORD pid;    /* Permanent guest PID, not a pointer. */
        WORD done;   /* Published completion after TLS teardown. */
        WORD phase;  /* Owner interval; no heap ordering or authority. */
    } result = {-1, 0, 0, 0, 0};

    DVM_ResidentCard card;
    unsigned int count;
    int more;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Processctl_residentstate, ret), sizeof(*call.ret));
    if (sizeof(result) != sizeof(*call.ret))
    {
        error(exBounds);
    }
    if (call.after >= 0 && dvm_resident_observe((unsigned long)call.after, &card, 1, &count, &more))
    {
        result.status = 0;
        if (count && card.id <= (unsigned long)LLONG_MAX)
        {
            result.status = 1;
            result.id = (LONG)card.id;
            result.pid = card.pid;
            result.done = card.done;
            result.phase = card.phase;
        }
        else if (count)
        {
            result.status = -1;
        }
    }
    dvm_sys_scalar_result(&result, call.ret, sizeof(result));
}


/* EN: Borrow rooted frame and stage one exact-ID copied activity. A reaped ID
 * returns zero/gone; invalid/unavailable returns -1/zero. Guard only scalar result
 * publication; never inspect Prog or extend target lifetime after the copy.
 * RU: Заимствовать rooted frame и сохранить copied activity по exact ID. Reaped ID
 * даёт zero/gone; invalid/unavailable даёт -1/zero. Guard только scalar publication;
 * не читать Prog и не продлевать target lifetime после копирования. */
void Processctl_residentactivity(void *frame)
{
    F_Processctl_residentactivity call;
    WORD result[2] = {-1, 0}; /* Staged status and copied activity; no managed references. */
    DVM_ResidentCard card;
    unsigned int count;
    int more;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Processctl_residentactivity, ret), sizeof(*call.ret));
    if (sizeof(result) != sizeof(*call.ret))
    {
        error(exBounds);
    }
    if (call.id > 0 && dvm_resident_observe((unsigned long)(call.id - 1), &card, 1, &count, &more))
    {
        result[0] = 0;
        if (count && card.id == (unsigned long)call.id)
        {
            result[0] = 1;
            result[1] = card.activity;
        }
    }
    dvm_sys_scalar_result(result, call.ret, sizeof(result));
}


/* EN: Borrow rooted call; stage exact-ID progress and publish scalar fields.
 * Missing/refusal/unrepresentable counters yield zero payload, never target access.
 * RU: Заимствовать rooted call; сохранить exact-ID progress и опубликовать scalars.
 * Missing/refusal/unrepresentable counters дают zero payload, без target access. */
void Processctl_residentprogress(void *frame)
{
    F_Processctl_residentprogress call;
    LONG result[4] = {-1, 0, 0, 0}; /* Staged status and three signed-big counters. */
    DVM_ResidentCard card;
    unsigned int count;
    int more;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Processctl_residentprogress, ret), sizeof(*call.ret));
    if (sizeof(result) != sizeof(*call.ret))
    {
        error(exBounds);
    }
    if (call.id > 0 && dvm_resident_observe((unsigned long)(call.id - 1), &card, 1, &count, &more))
    {
        result[0] = 0;
        if (count && card.id == (unsigned long)call.id)
        {
            result[0] = -1;
            if (card.dispatches <= (unsigned long)LLONG_MAX && card.turns <= card.dispatches &&
                card.waits <= (unsigned long)LLONG_MAX)
            {
                result[0] = 1;
                result[1] = (LONG)card.dispatches;
                result[2] = (LONG)card.turns;
                result[3] = (LONG)card.waits;
            }
        }
    }
    dvm_sys_scalar_result(result, call.ret, sizeof(result));
}


/* EN: Stage one coherent native registry summary, then publish guarded scalars.
 * Borrow rooted frame; unavailable/unrepresentable output is -1 plus zeros.
 * RU: Сохранить coherent native registry summary, затем опубликовать guarded scalars.
 * Заимствовать rooted frame; unavailable/unrepresentable даёт -1 и нули. */
void Processctl_residentsummary(void *frame)
{
    F_Processctl_residentsummary call;
    LONG result[6] = {-1, 0, 0, 0, 0, 0}; /* Status and guarded registry counters. */
    DVM_ResidentSummary summary;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Processctl_residentsummary, ret), sizeof(*call.ret));
    if (sizeof(result) != sizeof(*call.ret))
    {
        error(exBounds);
    }
    if (dvm_resident_summary(&summary) && summary.created <= (unsigned long)LLONG_MAX &&
        summary.registered <= (unsigned long)LLONG_MAX && summary.reaping <= (unsigned long)LLONG_MAX &&
        summary.reaped <= (unsigned long)LLONG_MAX && summary.done <= (unsigned long)LLONG_MAX)
    {
        result[0] = 1;
        result[1] = (LONG)summary.created;
        result[2] = (LONG)summary.registered;
        result[3] = (LONG)summary.reaping;
        result[4] = (LONG)summary.reaped;
        result[5] = (LONG)summary.done;
    }
    dvm_sys_scalar_result(result, call.ret, sizeof(result));
}


/* EN: Cancel a direct child's wait once; frame/result remain rooted while
 * publishing the staged outcome under cell guards. Invalid PID yields Denied.
 * RU: Один раз отменить ожидание прямого child; frame/result rooted во время
 * guarded publication сохранённого outcome. Неверный PID даёт Denied. */
void Processctl_cancelwait(void *frame)
{
    F_Processctl_cancelwait call;
    WORD result = Processctl_Denied;

    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Processctl_cancelwait, ret), sizeof(*call.ret));
    if (call.pid > 0 && call.pid <= INT_MAX)
    {
        result = dvm_runtime_cancel_child_wait(currun(), (int)call.pid);
    }
    dvm_sys_scalar_result(&result, call.ret, sizeof(result));
}


/* EN: Validate the prepared cancelwait frame before parking its caller.
 * RU: Проверить подготовленный кадр cancelwait перед парковкой вызывающего. */
int dvm_processctl_cancel_ready(void *frame)
{
    F_Processctl_cancelwait call;
    uintptr start = (uintptr)frame;
    uintptr top = (uintptr)DVM_R.SP;
    if (start < (uintptr)DVM_R.EX || top > (uintptr)DVM_R.TS || start > top || top - start != sizeof(call))
    {
        error(exBounds);
    }
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Processctl_cancelwait, ret), sizeof(*call.ret));
    return call.pid <= 0 || call.pid > INT_MAX || dvm_runtime_prepare_cancelwait(currun(), (int)call.pid);
}


/* EN: Request direct-child termination once from rooted frame. A native
 * continuation keeps ownership through cleanup; publish staged status with guards.
 * RU: Один раз запросить termination прямого child из rooted frame. Native
 * continuation держит ownership до cleanup; status публикуется под guards. */
void Processctl_kill(void *frame)
{
    F_Processctl_kill call;
    WORD result = Processctl_Denied;

    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Processctl_kill, ret), sizeof(*call.ret));
    if (call.pid > 0 && call.pid <= INT_MAX)
    {
        result = dvm_runtime_kill_child(currun(), (int)call.pid);
    }
    dvm_sys_scalar_result(&result, call.ret, sizeof(result));
}


/* EN: Validate borrowed killwait frame/scalars before MCALL publication.
 * Return ready or let the original caller park without consuming its result.
 * RU: Проверить borrowed killwait frame/scalars до MCALL publication. Вернуть
 * готовность либо припарковать исходного caller, не потребляя его result. */
int dvm_processctl_call_ready(void *frame)
{
    F_Processctl_killwait call;
    uintptr start, top, extent;

    start = (uintptr)frame;
    top = (uintptr)DVM_R.SP;
    extent = (uintptr)DVM_R.TS;
    if (start < (uintptr)DVM_R.EX || top > extent || start > top || top - start != sizeof(call))
    {
        error(exBounds);
    }
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Processctl_killwait, ret), sizeof(*call.ret));
    return call.pid <= 0 || call.pid > INT_MAX || dvm_runtime_prepare_killwait(currun(), (int)call.pid);
}


/* EN: Consume this prepared call's exact completion once, then publish its
 * staged scalar under guards. BUSY must not consume the completion a second time.
 * RU: Один раз потребить точный completion prepared call и опубликовать scalar
 * под guards. BUSY не должен потреблять completion повторно. */
void Processctl_killwait(void *frame)
{
    F_Processctl_killwait call;
    WORD result = Processctl_Denied;

    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Processctl_killwait, ret), sizeof(*call.ret));
    if (call.pid > 0 && call.pid <= INT_MAX)
    {
        result = dvm_runtime_killwait_result(currun(), (int)call.pid);
    }
    dvm_sys_scalar_result(&result, call.ret, sizeof(result));
}


/* EN/RU: Register generated hosted module once before any guest/owner starts.
 * Зарегистрировать generated hosted module один раз до старта guest/owner. */
void dvm_processctl_initialize(void)
{
    builtinmod("$Processctl", Processctlmodtab, Processctlmodlen);
}
