/* Copyright (c) 2026 bhgv
 * [GC:] EN: Weak exact-instance registry owns only native groups; finalizers queue preallocated native cleanup.
 * RU: Weak registry exact instances владеет лишь native groups; finalizers ставят preallocated native cleanup.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runqueue.h"
#include "environment.h"
#include "../vfs/launch/context.h"
#include "../vfs/sysevents.h"
#include "../close_worker.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>


/* [GC:] EN/RU: Weak link key is never a GC root; the finalizer removes it before that address becomes reusable.
 * Weak link key не является GC root; finalizer удаляет его до повторного использования адреса.
 */
typedef struct ModuleEnvironment
{
    DVM_LaunchContext *context; /* Optional owned immutable per-instance launch metadata. */
    Modlink *link; /* Comparison only under registry lock; never dereferenced on closer. */
    DVM_Namespace *group; /* Owned prepared namespace, borrowed only under a rooted link/frame. */
    DVM_CloseJob *job; /* Preallocated before attachment, transferred at final managed retirement. */
    uint64_t identity; /* Never-reused binding identity for process-local pctl variants. */
    struct ModuleEnvironment *next; /* Registry-owned chain, protected by lock. */
} ModuleEnvironment;


/* EN/RU: Detached per-process variant keeps no managed link pointer or ownership cycle.
 * Per-process variant не держит managed link pointer или ownership cycle.
 */
struct DVM_ModuleOverride
{
    uint64_t identity; /* Exact live binding token, not a module pathname or reused address. */
    DVM_Namespace *group; /* Owned pctl-created live namespace. */
    DVM_SysEvents *events; /* Owned independent process/binding inbox; never the caller's base inbox. */
    struct DVM_ModuleOverride *next; /* Permanent owner's native-only list. */
};
static pthread_mutex_t environment_lock = PTHREAD_MUTEX_INITIALIZER; /* Protects registry publication/removal only, never I/O. */
static ModuleEnvironment *environments; /* Weak keys; values own no managed references. */
static uint64_t next_identity; /* Binding tokens never wrap or get reused. */


/* EN/RU: Closer consumes detached native state after all managed borrowers are gone; payload owned on entry.
 * Closer поглощает detached native state после ухода managed borrowers; payload принадлежит callback.
 */
static void close_environment(void *payload)
{
    ModuleEnvironment *entry = payload;
    dvm_launch_context_release(entry->context);
    dvm_namespace_release(entry->group);
    free(entry);
}
static const DVM_VfsOps cleanup = {.close = close_environment}; /* Native-only deferred namespace destruction. */


/* [GC:] EN/RU: See environment.h; allocations precede publication and no registry mutex covers a destructor.
 * См. environment.h; allocation до publication, destructor вне registry mutex.
 */
int dvm_module_environment_bind_context(Modlink *link, DVM_Namespace *group, DVM_LaunchContext *context)
{
    if (!link || link == H || !group)
    {
        return 0;
    }
    ModuleEnvironment *entry = calloc(1, sizeof(*entry));
    DVM_CloseJob *job = entry ? dvm_close_prepare() : NULL;
    if (!entry || !job || !dvm_namespace_retain(group))
    {
        free(entry);
        dvm_close_discard(job);
        return 0;
    }
    if (context && !dvm_launch_context_retain(context))
    {
        dvm_namespace_release(group);
        free(entry);
        dvm_close_discard(job);
        return 0;
    }
    entry->context = context;
    entry->link = link;
    entry->group = group;
    entry->job = job;
    pthread_mutex_lock(&environment_lock);
    int valid = next_identity != UINT64_MAX;
    for (ModuleEnvironment *old = environments; old && valid; old = old->next)
    {
        valid = old->link != link;
    }
    if (valid)
    {
        entry->identity = ++next_identity;
        entry->next = environments;
        environments = entry;
    }
    pthread_mutex_unlock(&environment_lock);
    if (!valid)
    {
        dvm_close_discard(job);
        close_environment(entry);
    }
    return valid;
}


/* [GC:] EN/RU: See environment.h; weak removal and native ownership transfer happen before hosted Modlink destruction.
 * См. environment.h; weak removal/native ownership transfer до уничтожения hosted Modlink.
 */
void dvm_module_environment_retire(Modlink *link)
{
    pthread_mutex_lock(&environment_lock);
    ModuleEnvironment **cursor = &environments;
    while (*cursor && (*cursor)->link != link)
    {
        cursor = &(*cursor)->next;
    }
    ModuleEnvironment *entry = *cursor;
    if (entry)
    {
        *cursor = entry->next;
        entry->link = NULL;
    }
    pthread_mutex_unlock(&environment_lock);
    if (entry)
    {
        DVM_VfsHandle resource = {.ops = &cleanup, .state = entry};
        dvm_close_submit_resource(entry->job, &resource);
    }
}


/* [GC:] EN/RU: Find a rooted instance without extending managed lifetime; caller's ACTIVE frame pin outlives borrowed result.
 * Найти rooted instance без продления managed lifetime; ACTIVE frame pin caller живёт дольше borrowed result.
 */
static ModuleEnvironment *find(Modlink *link)
{
    pthread_mutex_lock(&environment_lock);
    ModuleEnvironment *entry = environments;
    while (entry && entry->link != link)
    {
        entry = entry->next;
    }
    pthread_mutex_unlock(&environment_lock);
    return entry;
}


/* [GC:] EN/RU: Walk live owner frames; metadata selects context-bearing entries, zero selects any bound namespace.
 * Only exact module crossings change the lookup key; no managed allocation or frame pointer escapes.
 * Обойти live frames permanent owner, включая builtin/local helpers, без saved Prog.R snapshots.
 */
static ModuleEnvironment *current(Prog *process, int metadata)
{
    if (!process || process != currun())
    {
        return NULL;
    }
    ModuleEnvironment *entry = find(DVM_R.M);
    if (metadata && entry && !entry->context)
    {
        entry = NULL;
    }
    const void *fp = DVM_R.FP;
    while (!entry && fp && fp != H)
    {
        Frame frame;
        memcpy(&frame, fp, sizeof(frame));
        if (frame.mr && frame.mr != H)
        {
            entry = find(frame.mr);
            if (metadata && entry && !entry->context)
            {
                entry = NULL;
            }
        }
        fp = frame.fp;
    }
    return entry;
}


/* [GC:] EN/RU: See environment.h; current/rooted entry pins group while this owner runs or waits with roots published.
 * См. environment.h; current/rooted entry удерживает group при execution и ожиданиях с published roots.
 */
DVM_Namespace *dvm_module_environment_current(Prog *process)
{
    ModuleEnvironment *entry = current(process, 0);
    if (!entry)
    {
        return NULL;
    }
    for (DVM_ModuleOverride *item = ((DVM_RuntimeProcess *)process)->module_namespaces; item; item = item->next)
    {
        if (item->identity == entry->identity && item->group)
        {
            return item->group;
        }
    }
    return entry->group;
}


/* [GC:] EN/RU: See environment.h; reserve only this owner's scalar slot, leaving effective namespace unchanged.
 * См. environment.h; резервировать только scalar slot owner, не меняя effective namespace.
 */
int dvm_module_environment_reserve(Prog *process)
{
    ModuleEnvironment *entry = current(process, 0);
    if (!entry)
    {
        return process && process == currun();
    }
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)process;
    for (DVM_ModuleOverride *item = owner->module_namespaces; item; item = item->next)
    {
        if (item->identity == entry->identity)
        {
            if (!item->events)
            {
                item->events = dvm_sysevents_create(item->group ? item->group : entry->group);
            }
            return item->events != NULL;
        }
    }
    DVM_ModuleOverride *item = calloc(1, sizeof(*item));
    if (!item)
    {
        return 0;
    }
    item->identity = entry->identity;
    item->events = dvm_sysevents_create(entry->group);
    if (!item->events)
    {
        free(item);
        return 0;
    }
    item->next = owner->module_namespaces;
    owner->module_namespaces = item;
    return 1;
}


/* EN/RU: See environment.h; only this ACTIVE owner's scalar slot is read or initialized.
 * См. environment.h; читается/создаётся лишь слот своего ACTIVE потока, без чужих кадров.
 */
DVM_SysEvents *dvm_module_events_current(Prog *process)
{
    if (!process || process != currun())
    {
        return NULL;
    }
    ModuleEnvironment *entry = current(process, 0);
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)process;
    if (!entry)
    {
        return owner->system_events;
    }
    if (!dvm_module_environment_reserve(process))
    {
        return NULL;
    }
    for (DVM_ModuleOverride *item = owner->module_namespaces; item; item = item->next)
    {
        if (item->identity == entry->identity)
        {
            return item->events;
        }
    }
    return NULL;
}


/* [GC:] EN/RU: See environment.h; preparation can fail only before the permanent owner's pointer changes.
 * См. environment.h; подготовка отказывает только до изменения указателя permanent owner.
 */
int dvm_module_environment_replace(Prog *process, DVM_Namespace *replacement)
{
    if (!process || process != currun() || !replacement)
    {
        return 0;
    }
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)process;
    ModuleEnvironment *entry = current(process, 0);
    if (!entry)
    {
        if (!dvm_sysevents_group(owner->system_events, replacement))
        {
            return 0;
        }
        DVM_Namespace *old = owner->namespace_group;
        owner->namespace_group = replacement;
        dvm_namespace_release(old);
        return 1;
    }
    if (!dvm_module_environment_reserve(process))
    {
        return 0;
    }
    DVM_ModuleOverride *item = owner->module_namespaces;
    while (item && item->identity != entry->identity)
    {
        item = item->next;
    }
    if (!item || !dvm_sysevents_group(item->events, replacement))
    {
        return 0;
    }
    DVM_Namespace *old = item->group;
    item->group = replacement;
    dvm_namespace_release(old);
    return 1;
}


/* EN/RU: See environment.h; process constructor failure and final owner retirement share this native-only cleanup.
 * См. environment.h; constructor failure и final retirement используют общий native-only cleanup.
 */
void dvm_module_environment_release(Prog *process)
{
    if (process)
    {
        DVM_ModuleOverride **head = &((DVM_RuntimeProcess *)process)->module_namespaces;
        while (*head)
        {
            DVM_ModuleOverride *item = *head;
            *head = item->next;
            dvm_sysevents_retire(item->events);
            dvm_sysevents_release(item->events);
            dvm_namespace_release(item->group);
            free(item);
        }
    }
}


/* [GC:] EN/RU: See environment.h; group-only attachment deliberately inherits an enclosing accepted launch context.
 * См. environment.h; group-only attachment намеренно наследует enclosing accepted launch context.
 */
int dvm_module_environment_bind(Modlink *link, DVM_Namespace *group)
{
    return dvm_module_environment_bind_context(link, group, NULL);
}


/* [GC:] EN/RU: See environment.h; owning Modlink frames or process constructor pins keep this native snapshot alive.
 * См. environment.h; Modlink frames либо process constructor pins удерживают native snapshot.
 */
DVM_LaunchContext *dvm_module_launch_current(Prog *process)
{
    if (!process || process != currun())
    {
        return NULL;
    }
    ModuleEnvironment *entry = current(process, 1);
    return entry ? entry->context : ((DVM_RuntimeProcess *)process)->launch_context;
}


/* [GC:] EN/RU: See environment.h; scalar binding identity avoids reselecting the original module group in a child's first frame.
 * Only this active binding is inherited; unrelated per-module overrides are not copied into another execution context.
 * RU: Контракт в environment.h; идентификатор привязки сохраняет выбранную группу в первом кадре ребёнка.
 * Наследуется только текущая привязка, остальные переопределения модулей не копируются.
 */
int dvm_module_environment_inherit(Prog *child, Prog *parent)
{
    if (!child || child == parent || ((DVM_RuntimeProcess *)child)->module_namespaces)
    {
        return 0;
    }
    if (!parent || parent != currun())
    {
        return 1;
    }
    ModuleEnvironment *entry = current(parent, 0);
    if (!entry)
    {
        return 1;
    }
    DVM_Namespace *group = dvm_module_environment_current(parent);
    DVM_ModuleOverride *copy = group ? calloc(1, sizeof(*copy)) : NULL;
    if (!copy || !dvm_namespace_retain(group))
    {
        free(copy);
        return 0;
    }
    copy->identity = entry->identity;
    copy->group = group;
    copy->events = dvm_sysevents_create(group);
    if (!copy->events)
    {
        dvm_namespace_release(group);
        free(copy);
        return 0;
    }
    ((DVM_RuntimeProcess *)child)->module_namespaces = copy;
    return 1;
}
