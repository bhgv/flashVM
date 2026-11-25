/* Copyright (c) 2026 bhgv
 * EN: Universal per-launch host dispatch; a bound job never falls back to another/local UI session.
 * RU: Универсальный per-launch host-диспетчер; привязанное задание не использует чужой/local UI fallback.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "unibusmod.h"
#include "runqueue.h"
#include "sys_capture.h"
#include "disvm/unibus_binding.h"
#include <stddef.h>
#ifdef DVM_RUNTIME_EDITOR
#include "local.h"
#endif


/* EN: Borrow the active caller's immutable native binding and typed frame; 1 handled/0 absent.
 * Admission failure raises on this caller, never retries/falls back. Foreign attach returns -1
 * through its ordinary typed result. The process pins native state through the call/error boundary.
 * RU: Заимствовать binding своего active process и typed frame; 1 выполнено/0 нет привязки.
 * Отказ не вызывает retry/fallback. Чужой attach возвращает -1 обычным typed-результатом.
 */
static int dispatched(unsigned method, void *frame)
{
    Prog *process = currun(); /* Current permanent guest, never a foreign process lookup. */
    UnibusBinding *binding = process ? ((DVM_RuntimeProcess *)process)->unibus : NULL;
    if (!binding)
    {
        if (process && ((DVM_RuntimeProcess *)process)->unibus_isolated)
        {
            error("Unibus is not bound to this process");
        }
        return 0;
    }
    int result = unibusinvoke(binding, (uintptr_t)process, process->pid, method, frame); /* Owner-local synchronous call. */
    if (result == 1)
    {
        return 1;
    }
    if (result == -1 && method == UNIBUS_ATTACH)
    {
        F_Unibus_attach call; /* Captured root-owned return cell. */
        WORD refused = -1; /* Standard attach refusal, without touching another owner's ports. */
        dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Unibus_attach, ret), sizeof(*call.ret));
        dvm_sys_scalar_result(&refused, call.ret, sizeof(refused));
        return 1;
    }
    error(result == -2 ? "retired Unibus binding" : result == -3 ?
        "Unibus operation unavailable for this launch" : "Unibus operation belongs to the launch owner");
    return 1;
}


/* EN/RU: Dispatch the existing ink .m frame on its caller's native owner.
 * Передать существующий frame ink на native owner вызывающего process.
 */
void Unibus_ink(void *frame)
{
    if (!dispatched(UNIBUS_INK, frame))
    {
#ifdef DVM_RUNTIME_EDITOR
        dvm_local_Unibus_ink(frame);
#else
        error("Unibus is not bound to this process");
#endif
    }
}


/* EN/RU: Dispatch the existing inkadmit .m frame on its caller's native owner.
 * Передать существующий frame inkadmit на native owner вызывающего process.
 */
void Unibus_inkadmit(void *frame)
{
    if (!dispatched(UNIBUS_INKADMIT, frame))
    {
#ifdef DVM_RUNTIME_EDITOR
        dvm_local_Unibus_inkadmit(frame);
#else
        error("Unibus is not bound to this process");
#endif
    }
}


/* EN/RU: Dispatch the existing inkdone .m frame on its caller's native owner.
 * Передать существующий frame inkdone на native owner вызывающего process.
 */
void Unibus_inkdone(void *frame)
{
    if (!dispatched(UNIBUS_INKDONE, frame))
    {
#ifdef DVM_RUNTIME_EDITOR
        dvm_local_Unibus_inkdone(frame);
#else
        error("Unibus is not bound to this process");
#endif
    }
}


/* EN/RU: Dispatch the existing linenumbers .m frame on its caller's native owner.
 * Передать существующий frame linenumbers на native owner вызывающего process.
 */
void Unibus_linenumbers(void *frame)
{
    if (!dispatched(UNIBUS_LINES, frame))
    {
#ifdef DVM_RUNTIME_EDITOR
        dvm_local_Unibus_linenumbers(frame);
#else
        error("Unibus is not bound to this process");
#endif
    }
}


/* EN/RU: Dispatch the existing rows .m frame on its caller's native owner.
 * Передать существующий frame rows на native owner вызывающего process.
 */
void Unibus_rows(void *frame)
{
    if (!dispatched(UNIBUS_ROWS, frame))
    {
#ifdef DVM_RUNTIME_EDITOR
        dvm_local_Unibus_rows(frame);
#else
        error("Unibus is not bound to this process");
#endif
    }
}


/* EN/RU: Dispatch the existing attach .m frame on its caller's native owner.
 * Передать существующий frame attach на native owner вызывающего process.
 */
void Unibus_attach(void *frame)
{
    if (!dispatched(UNIBUS_ATTACH, frame))
    {
#ifdef DVM_RUNTIME_EDITOR
        dvm_local_Unibus_attach(frame);
#else
        error("Unibus is not bound to this process");
#endif
    }
}


/* EN/RU: Dispatch the existing attachbulk .m frame on its caller's native owner.
 * Передать существующий frame attachbulk на native owner вызывающего process.
 */
void Unibus_attachbulk(void *frame)
{
    if (!dispatched(UNIBUS_ATTACHBULK, frame))
    {
#ifdef DVM_RUNTIME_EDITOR
        dvm_local_Unibus_attachbulk(frame);
#else
        error("Unibus is not bound to this process");
#endif
    }
}


/* EN/RU: Dispatch the existing detach .m frame on its caller's native owner.
 * Передать существующий frame detach на native owner вызывающего process.
 */
void Unibus_detach(void *frame)
{
    if (!dispatched(UNIBUS_DETACH, frame))
    {
#ifdef DVM_RUNTIME_EDITOR
        dvm_local_Unibus_detach(frame);
#else
        error("Unibus is not bound to this process");
#endif
    }
}


/* EN/RU: Dispatch the existing fresh .m frame on its caller's native owner.
 * Передать существующий frame fresh на native owner вызывающего process.
 */
void Unibus_fresh(void *frame)
{
    if (!dispatched(UNIBUS_FRESH, frame))
    {
#ifdef DVM_RUNTIME_EDITOR
        dvm_local_Unibus_fresh(frame);
#else
        error("Unibus is not bound to this process");
#endif
    }
}


/* EN/RU: Dispatch the existing path .m frame on its caller's native owner.
 * Передать существующий frame path на native owner вызывающего process.
 */
void Unibus_path(void *frame)
{
    if (!dispatched(UNIBUS_PATH, frame))
    {
#ifdef DVM_RUNTIME_EDITOR
        dvm_local_Unibus_path(frame);
#else
        error("Unibus is not bound to this process");
#endif
    }
}


/* EN: Forward opening to the active mounted launch only.
 * RU: Передать rpcopen только активному смонтированному запуску. */
void Unibus_rpcopen(void *frame)
{
    if (!dispatched(UNIBUS_RPCOPEN, frame))
    {
        error("StyxRPC is not bound to this process");
    }
}


/* EN: Forward sending to the active mounted launch only.
 * RU: Передать rpcsend только активному смонтированному запуску. */
void Unibus_rpcsend(void *frame)
{
    if (!dispatched(UNIBUS_RPCSEND, frame))
    {
        error("StyxRPC is not bound to this process");
    }
}


/* EN: Forward receiving to the active mounted launch only.
 * RU: Передать rpcrecv только активному смонтированному запуску. */
void Unibus_rpcrecv(void *frame)
{
    if (!dispatched(UNIBUS_RPCRECV, frame))
    {
        error("StyxRPC is not bound to this process");
    }
}


/* EN: Forward closing to the active mounted launch only.
 * RU: Передать rpcclose только активному смонтированному запуску. */
void Unibus_rpcclose(void *frame)
{
    if (!dispatched(UNIBUS_RPCCLOSE, frame))
    {
        error("StyxRPC is not bound to this process");
    }
}


/* EN/RU: Register the shared interface once before any guest modules/threads are started.
 * Зарегистрировать общий интерфейс до запуска guest-модулей и потоков.
 */
void dvm_unibus_initialize(void)
{
    if (builtinmod("$Unibus", Unibusmodtab, Unibusmodlen) == nil)
    {
        error("cannot initialize Unibus interface");
    }
}
