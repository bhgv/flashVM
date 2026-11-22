/* Copyright (c) 2026 bhgv
 * EN: Resident error/FP state and inherited native namespace ownership.
 * RU: Error/FP-состояние resident и владение наследуемым native namespace.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "runqueue.h"
#include "process_environment.h"
#include "vfs/launch/runtime.h"
#include "sys_capture.h"
#include "vfs/boot.h"
#include "vfs/guest.h"
#include "module_interface/environment.h"
#include "vfs/sysevents.h"
#include "disvm/native_process.h"
#include "disvm/unibus_binding.h"
#include "vfs/cancel.h"
#include "vfs/scalar_owner_internal.h"
#include "../native/terminal/unibus_inbox.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN: Initialize private process before publication; parent is borrowed from
 * the calling owner. Share namespace reference, copy error/FP state and retain inherited terminal grant.
 * Initial resident retains startup group; zero on missing/overflowed reference.
 * RU: Инициализировать приватный process до публикации; parent заимствован
 * своим owner. Разделить namespace, скопировать error/FP state и удержать inherited terminal grant.
 * Начальный resident удерживает startup group; ноль при missing/overflowed
 * reference, без публикации. */
int dvm_process_environment_init(Prog *process, Prog *parent)
{
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)process;
    Osenv *environment = &((DVM_RuntimeProcess *)process)->environment;
    Osenv *source = parent ? parent->osenv : NULL;
    memset(environment, 0, sizeof(*environment));
    environment->errstr = environment->errbuf1;
    environment->syserrstr = environment->errbuf0;
    if (source && source->errstr)
    {
        snprintf(environment->errstr, ERRMAX, "%s", source->errstr);
    }
    if (source && source->syserrstr)
    {
        snprintf(environment->syserrstr, ERRMAX, "%s", source->syserrstr);
    }
    DVM_RuntimeProcess *source_owner = (DVM_RuntimeProcess *)parent;
    DVM_Namespace *group = parent ? (parent == currun() && source_owner->remote_call_handoff != 1 ?
        dvm_namespace_current_group() : source_owner->namespace_group) : dvm_namespace_boot_group();
    if (!group || !dvm_namespace_retain(group))
    {
        return 0;
    }
    owner->system_events = dvm_sysevents_create(group);
    if (!owner->system_events)
    {
        dvm_namespace_release(group);
        return 0;
    }
    owner->namespace_group = group;
    process->osenv = environment;
    owner->unibus = parent ? unibusinherit(((DVM_RuntimeProcess *)parent)->unibus) : NULL;
    owner->module_local_role = parent ? ((DVM_RuntimeProcess *)parent)->module_local_role : DVM_ORIGIN_TERMINAL;
    owner->provider_execution = parent ? ((DVM_RuntimeProcess *)parent)->provider_execution : 0;
    owner->unibus_isolated = parent ? ((DVM_RuntimeProcess *)parent)->unibus_isolated : 0;
    if (!dvm_module_environment_inherit(process, parent) || !dvm_launch_process_inherit(process, parent))
    {
        dvm_process_environment_destroy(process);
        return 0;
    }
    if (fegetenv(&((DVM_RuntimeProcess *)process)->floating_environment))
    {
        abort();
    }
    return 1;
}


/* EN: Consume namespace after failed private setup or final owner retirement;
 * process may be NULL or partly initialized. No managed references or throwing.
 * RU: Освободить namespace после отказа подготовки либо retirement владельца;
 * process допустим NULL/частичный. Без managed references и исключений. */
void dvm_process_environment_destroy(Prog *process)
{
    if (process)
    {
        DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)process;
        unibusownerretired(owner->unibus, (uintptr_t)process);
        unibusrelease(owner->unibus);
        owner->unibus = NULL;
        dvm_launch_process_release(process);
        dvm_sysevents_retire(owner->system_events);
        dvm_sysevents_release(owner->system_events);
        owner->system_events = NULL;
        dvm_module_environment_release(process);
        dvm_scalar_discard_staged(process);
        dvm_sysevents_retire(owner->remote_call_events);
        dvm_sysevents_release(owner->remote_call_events);
        owner->remote_call_events = NULL;
        dvm_cancel_release(owner->remote_call_cancel);
        owner->remote_call_cancel = NULL;
        styxunibusmailboxseal(owner->remote_call_reply);
        dvm_provider_release(owner->remote_call_reply);
        owner->remote_call_reply = NULL;
        dvm_provider_release(owner->remote_call_context);
        owner->remote_call_context = NULL;
        dvm_namespace_release(owner->remote_call_group);
        owner->remote_call_group = NULL;
        dvm_namespace_release(owner->namespace_group);
        owner->namespace_group = NULL;
    }
}


/* EN: Select the current native owner's module inbox; return a borrowed inbox/NULL, with no foreign frames.
 * RU: Выбрать очередь модуля своего нативного потока; вернуть заимствованную очередь/NULL, без чужих кадров. */
static DVM_SysEvents *current_events(void)
{
    return dvm_module_events_current(currun());
}


/* EN: Bind the permanent owner's borrowed process; no context/jump-frame copy.
 * Return one/zero for valid/invalid metadata; FP runtime failure aborts. No roots
 * or frames change, and no native ownership transfers to another process.
 * RU: Привязать process своего постоянного owner без копии context/jump frames.
 * Один/ноль для valid/invalid metadata; отказ FP runtime завершает host. Корни
 * и frames не меняются, владение другому process не передаётся. */
int dvm_process_environment_bind(Prog *process)
{
    if (!up || !process || process->osenv != &((DVM_RuntimeProcess *)process)->environment)
    {
        return 0;
    }
    if (fegetenv(&dvm_native_execution()->host_fp_environment) ||
        fesetenv(&((DVM_RuntimeProcess *)process)->floating_environment))
    {
        abort();
    }
    up->env = process->osenv;
    dvm_sysevents_bind(((DVM_RuntimeProcess *)process)->system_events);
    dvm_sysevents_select(current_events);
    return 1;
}


/* EN: Save borrowed process FP, restore host FP/default error environment before
 * destructors/native waits. No allocation; an FP runtime failure aborts.
 * RU: Сохранить FP process, вернуть host FP/default error environment перед
 * destructors/native waits. Без allocation; отказ FP runtime завершает host. */
void dvm_process_environment_release(Prog *process)
{
    if (fegetenv(&((DVM_RuntimeProcess *)process)->floating_environment) ||
        fesetenv(&dvm_native_execution()->host_fp_environment))
    {
        abort();
    }
    dvm_sysevents_bind(NULL);
    up->env = &up->defenv;
}


/* EN: Store one mapped guest string as this logical Prog's last host error. Capture
 * the original return span before string conversion can allocate/borrow REG.
 * Frame/source remain rooted by the claimed turn; no native release occurs.
 * Publish the full native WORD zero only after conversion and bounded copy.
 * RU: Сохранить mapped guest string как последнюю host error своего Prog.
 * Захватить return span до conversion; frame/source укоренены ACTIVE, без
 * native release. Записать WORD zero после bounded copy; conversion может raise. */
void Sys_werrstr(void *frame)
{
    F_Sys_werrstr call;
    char *destination;
    char *text;
    WORD result = 0;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Sys_werrstr, ret), sizeof(*call.ret));
    destination = up->env->errstr;
    text = string2c(call.s);
    snprintf(destination, ERRMAX, "%s", text);
    memmove(call.ret, &result, sizeof(result));
}


/* EN: Copy this bound OS owner's current logical error without consuming it. err
 * borrows size writable bytes; clamp to ERRMAX and preserve termination. No
 * allocation, managed access or throw; startup cleanup also permits absent up.
 * RU: Скопировать текущую ошибку owner в err вместимостью size, ограничив
 * ERRMAX и сохранив NUL. Без allocation/managed access/throw; up допустим NULL. */
void kgerrstr(char *err, uint size)
{
    const char *source = "<no-up>";
    if (!err || !size)
    {
        return;
    }
    if (size > ERRMAX)
    {
        size = ERRMAX;
    }
    if (up && up->env && up->env->errstr)
    {
        source = up->env->errstr;
    }
    snprintf(err, size, "%s", source);
}
