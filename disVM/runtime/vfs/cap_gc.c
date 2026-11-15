/* Copyright (c) 2026 bhgv
 * [GC:] EN: Unforgeable VFS shells with preallocated native close-worker retirement.
 * RU: Unforgeable VFS shells с preallocated retirement через native close-worker.
 */
#include "cap_internal.h"
#include "cap_gc.h"
#include "disvm/refcount.h"
#include "vfsexport_table.h"


/* [GC:] EN/RU: Guest-visible prefix is inert; private payload owns native-only cleanup.
 * Guest-visible prefix не даёт authority; private payload владеет native cleanup. */
typedef struct CapShell
{
    WORD reserved; /* Non-authoritative public field; changing it grants nothing. */
    DVM_CloseJob *job; /* Preallocated finalizer retirement node. */
    DVM_VfsHandle cleanup; /* Native state ownership, never a managed pointer. */
} CapShell;
static unsigned long created_states; /* Atomic successful native state construction count. */
static unsigned long retired_states; /* Atomic completed closer retirement count. */
static Type *types[DVM_CAP_TYPES]; /* Immutable exact descriptors initialized before guests. */


/* EN/RU: Retire one detached native state on the closer, outside collector/VM locks.
 * Освободить detached native state на closer, вне collector/VM locks. */
static void close_state(void *payload)
{
    DVM_CapState *state = payload;
    if (state->service)
    {
        DVM_CapService *service = state->service;
        if (state->kind == DVM_CAP_OWNER || state->kind == DVM_CAP_REQUEST)
        {
            fvm_port_sync_lock(service->lock);
            if (service->publication)
            {
                if (state->kind == DVM_CAP_OWNER)
                {
                    dvm_publication_close(service->publication);
                    service->publication = NULL;
                }
                else
                {
                    dvm_publication_reply(service->publication, state->request.id, NULL);
                }
            }
            fvm_port_sync_unlock(service->lock);
        }
        dvm_cap_service_drop(service);
    }
    dvm_export_release(state->ticket);
    dvm_provider_release(state->root);
    fvm_port_sync_destroy(state->lock);
    free(state);
    __atomic_add_fetch(&retired_states, 1, __ATOMIC_RELAXED);
}
static const DVM_VfsOps cleanup_ops = {NULL, NULL, NULL, NULL, NULL, close_state}; /* Closer-only consuming operation. */


/* [GC:] EN/RU: Finalizer moves native ownership into its preallocated node, never closes inline.
 * Finalizer переносит native ownership в preallocated node, без inline I/O. */
static void finalize(Heap *heap, int swept)
{
    CapShell *shell = H2D(CapShell *, heap);
    USED(swept);
    if (shell->job)
    {
        DVM_CloseJob *job = shell->job;
        shell->job = NULL;
        dvm_close_submit_resource(job, &shell->cleanup);
    }
}


/* [GC:] EN/RU: See cap_gc.h; no guest-supplied descriptor can satisfy this identity check.
 * См. cap_gc.h; guest descriptor не может пройти exact identity check. */
int dvm_cap_gc_eligible(const Heap *heap)
{
    for (unsigned kind = 0; kind < DVM_CAP_TYPES; kind++)
    {
        if (types[kind] && heap->t == types[kind])
        {
            return 1;
        }
    }
    return 0;
}


/* [GC:] EN/RU: See cap_gc.h; collector already owns zero-count scalar shell after reader grace.
 * См. cap_gc.h; collector владеет zero-count scalar shell после reader grace. */
void dvm_cap_gc_finalize(Heap *heap)
{
    if (!dvm_cap_gc_eligible(heap) || heap->t->np || dvm_refcount_load_u(&heap->ref))
    {
        abort();
    }
    finalize(heap, 0);
}


/* [GC:] EN/RU: See internal header; no native effect precedes rooted managed staging.
 * См. internal header; native effects следуют только после rooted staging. */
void *dvm_cap_new(unsigned kind, DVM_CapState **out)
{
    CapShell *shell;
    DVM_CapState *state;
    DVM_CloseJob *job;
    *out = NULL;
    shell = H2D(CapShell *, heapz(types[kind]));
    volatile int rooted = 0;
    if (waserror())
    {
        if (rooted)
        {
            ptrdel(D2H(shell));
        }
        destroy(shell);
        nexterror();
    }
    ptradd(D2H(shell));
    rooted = 1;
    state = calloc(1, sizeof(*state));
    job = dvm_close_prepare();
    if (state)
    {
        state->lock = fvm_port_sync_create();
    }
    if (!state || !state->lock || !job)
    {
        if (state)
        {
            fvm_port_sync_destroy(state->lock);
        }
        free(state);
        dvm_close_discard(job);
        poperror();
        ptrdel(D2H(shell));
        destroy(shell);
        return H;
    }
    state->kind = kind;
    shell->job = job;
    shell->cleanup.ops = &cleanup_ops;
    shell->cleanup.state = state;
    *out = state;
    __atomic_add_fetch(&created_states, 1, __ATOMIC_RELAXED);
    poperror();
    return shell;
}


/* [GC:] EN/RU: See internal header; caller's new shell is rooted until result replacement.
 * См. internal header; new shell rooted до замены result. */
void dvm_cap_publish(void *value, void *destination)
{
    void *previous = H;
    if (waserror())
    {
        if (value != H)
        {
            ptrdel(D2H(value));
            destroy(value);
        }
        nexterror();
    }
    const char *problem = dvm_runtime_movp_copy(&value, destination, &previous);
    if (problem)
    {
        error((char *)problem);
    }
    destroy(previous);
    if (value != H)
    {
        ptrdel(D2H(value));
        destroy(value);
    }
    poperror();
}


/* [GC:] EN/RU: See internal header; used only while caller still owns the private root.
 * См. internal header; caller ещё владеет private root. */
void dvm_cap_abandon(void *value)
{
    if (value != H)
    {
        ptrdel(D2H(value));
        destroy(value);
    }
}


/* [GC:] EN/RU: See internal header; borrowed native payload stays pinned by captured argument.
 * См. internal header; native payload удерживается captured rooted argument. */
DVM_CapState *dvm_cap_get(void *value, unsigned kind)
{
    if (!value || value == H)
    {
        return NULL;
    }
    if (D2H(value)->t != types[kind])
    {
        error(exHeap);
    }
    return ((CapShell *)value)->cleanup.state;
}


/* EN/RU: See internal header; embedded NUL/empty/oversize input refuses before effects.
 * См. internal header; NUL/empty/oversize input отклоняется до effects. */
int dvm_cap_text(String *value, char *out, size_t capacity)
{
    extern char *dvm_runtime_file_path(String *name);
    char *text = dvm_runtime_file_path(value);
    if (!text || !*text || strlen(text) >= capacity)
    {
        return 0;
    }
    strcpy(out, text);
    return 1;
}


/* EN/RU: See cap_gc.h; no late type initialization or guest ABI mutation.
 * См. cap_gc.h; без late initialization и изменения guest ABI. */
void dvm_cap_initialize(void)
{
    for (unsigned kind = 0; kind < DVM_CAP_TYPES; kind++)
    {
        types[kind] = dtype(finalize, sizeof(CapShell), NULL, 0);
        if (!types[kind])
        {
            error(exNomem);
        }
    }
    builtinmod("$Vfsexport", Vfsexportmodtab, Vfsexportmodlen);
}


/* EN/RU: See cap_gc.h; equality is meaningful after native close-worker drain.
 * См. cap_gc.h; equality имеет смысл после native close-worker drain. */
void dvm_cap_lifetime_snapshot(unsigned long *created, unsigned long *retired)
{
    if (created)
    {
        *created = __atomic_load_n(&created_states, __ATOMIC_RELAXED);
    }
    if (retired)
    {
        *retired = __atomic_load_n(&retired_states, __ATOMIC_RELAXED);
    }
}
