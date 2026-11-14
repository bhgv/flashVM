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
