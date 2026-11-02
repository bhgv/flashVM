/* Real OS TLS and FlashVM mutexes: no green-thread scheduler in this layer. */
#include "disvm/native_context.h"
#include "channel_sync.h"
#include "native_owner_private.h"
#include <stdlib.h>
#include <limits.h>

#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600 /* InitOnce is available from Windows Vista. */
#endif
#include <windows.h>

#else
#include <pthread.h>

#endif


/* [GC:] This owns only native register storage; roots/process lifetime remain
 * the Dis host's responsibility. Context-local lock protects exclusive binding. */
struct DVM_NativeContext
{
    FVM_PortSync *sync;   /* Protect exclusive bind/unbind ownership. */
    void *storage;        /* Owned register-only or extended-up allocation. */
    void *host_process;   /* Extended up base, null for the legacy generic constructor. */
    void *registers;      /* Borrowed aligned subobject of storage. */
    size_t register_size; /* Exact extent checked by the typed REG adapter. */
    void *runtime;        /* Borrowed for the complete context lifetime. */
    void *process;        /* Generic constructor's current-process storage. */
    void **process_slot;  /* Generic process field or extended up.iprog subobject. */
    void *turn;           /* Borrowed native scheduler identity; prevents unbind until exact release. */
    DVM_NativePoll poll;  /* Owner's safepoint hook, if registered. */
    void *poll_user;      /* Borrowed until explicit hook removal. */
    DVM_HeapOwner heap_owner; /* Private TLS lifecycle/counters; no managed owners. */
    unsigned long owner_scopes; /* Owner-local ledgers; close before TLS unbind. */
    unsigned long object_locks; /* Owner-only guards; cannot survive safepoint/turn/unbind. */
    int bound;            /* Nonzero while one native TLS slot owns this context. */
};

#ifdef _WIN32
static INIT_ONCE tls_once = INIT_ONCE_STATIC_INIT;
static DWORD tls_key = TLS_OUT_OF_INDEXES;


/* Initialize one process-lifetime Windows TLS slot; never silently share R. */
static BOOL CALLBACK initialize_tls(PINIT_ONCE once, PVOID parameter, PVOID *result)
{
    (void)once;
    (void)parameter;
    (void)result;
    tls_key = TlsAlloc();
    return tls_key != TLS_OUT_OF_INDEXES;
}


/* Read/set only this OS thread's slot; propagate TLS allocation failures. */
static int tls_ready(void)
{
    return InitOnceExecuteOnce(&tls_once, initialize_tls, 0, 0) != 0;
}


/* Borrow the current thread's Windows slot; no VM or heap access occurs. */
static void *tls_get(void)
{
    return tls_ready() ? TlsGetValue(tls_key) : 0;
}


/* Publish only to this thread; a failed write leaves binding ownership unchanged. */
static int tls_set(void *value)
{
    return tls_ready() && TlsSetValue(tls_key, value);
}
#else
static pthread_once_t tls_once = PTHREAD_ONCE_INIT;
static pthread_key_t tls_key;
static int tls_status = -1;


/* One process-lifetime POSIX TLS key shared by Linux, macOS and Android. */
static void initialize_tls(void)
{
    tls_status = pthread_key_create(&tls_key, 0);
}


/* Explicit unbind owns cleanup; a thread destructor must not collect VM roots. */
static int tls_ready(void)
{
    return pthread_once(&tls_once, initialize_tls) == 0 && tls_status == 0;
}


/* Borrow the current native thread's POSIX slot, never a process-global value. */
static void *tls_get(void)
{
    return tls_ready() ? pthread_getspecific(tls_key) : 0;
}


/* Propagate failed TLS publication so bind can roll back before returning. */
static int tls_set(void *value)
{
    return tls_ready() && pthread_setspecific(tls_key, value) == 0;
}
#endif


/* Allocate no global VM state; every context owns a distinct zeroed register file. */
DVM_NativeContext *dvm_native_context_create(size_t register_size, void *runtime)
{
    DVM_NativeContext *context;
    if (!register_size)
    {
        return 0;
    }
    context = (DVM_NativeContext *)calloc(1, sizeof(*context));
    if (!context)
    {
        return 0;
    }
    context->sync = fvm_port_sync_create();
    context->storage = calloc(1, register_size);
    context->registers = context->storage;
    context->runtime = runtime;
    context->register_size = register_size;
    context->process_slot = &context->process;
    if (!context->sync || !context->registers)
    {
        if (context->sync)
        {
            fvm_port_sync_destroy(context->sync);
        }
        free(context->storage);
        free(context);
        return 0;
    }
    return context;
}


/* Caller must quiesce users before destroy; bound/turn checks refuse live owners.
 * context is native storage, not a managed root; NULL destruction is a no-op. */
int dvm_native_context_destroy(DVM_NativeContext *context)
{
    if (!context)
    {
        return 1;
    }
    fvm_port_sync_lock(context->sync);
    if (context->bound || context->turn || context->object_locks)
    {
        fvm_port_sync_unlock(context->sync);
        return 0;
    }
    fvm_port_sync_unlock(context->sync);
    fvm_port_sync_destroy(context->sync);
    free(context->storage);
    free(context);
    return 1;
}


/* Reserve one context for one native thread, rolling back a failed TLS publish. */
int dvm_native_context_bind(DVM_NativeContext *context)
{
    int ok;
    if (!context || !tls_ready() || tls_get())
    {
        return 0;
    }
    fvm_port_sync_lock(context->sync);
    if (context->bound)
    {
        fvm_port_sync_unlock(context->sync);
        return 0;
    }
    ok = tls_set(context);
    if (ok)
    {
        context->bound = 1;
    }
    fvm_port_sync_unlock(context->sync);
    return ok;
}


/* Detach only this owner after poll/world/turn reservations retire. An empty
 * process root slot alone does not prove the scheduler continuation is finished. */
int dvm_native_context_unbind(DVM_NativeContext *context)
{
    if (context && tls_get() == context && context->owner_scopes)
    {
        return 0;
    }
    if (dvm_native_context_object_count(context))
    {
        return 0;
    }
    if (!context || tls_get() != context || context->poll || context->heap_owner.world || context->turn)
    {
        return 0;
    }
    fvm_port_sync_lock(context->sync);
    if (!tls_set(0))
    {
        fvm_port_sync_unlock(context->sync);
        return 0;
    }
    *context->process_slot = 0;
    context->bound = 0;
    fvm_port_sync_unlock(context->sync);
    return 1;
}


/* Never infer ownership from a global current-process variable. */
DVM_NativeContext *dvm_native_context_current(void)
{
    return (DVM_NativeContext *)tls_get();
}


/* Reserve one native scheduler turn, independently of the mutable root slot.
 * The bound owner alone writes turn, so no foreign TLS or heap is accessed. */
int dvm_native_context_acquire_turn(DVM_NativeContext *context, void *token)
{
    if (dvm_native_context_object_count(context))
    {
        return 0;
    }
    if (!context || tls_get() != context || !token || context->turn)
    {
        return 0;
    }
    context->turn = token;
    return 1;
}


/* Release exactly the bound owner's borrowed native token. Caller guarantees
 * its scheduler/continuation has stopped using it; this does not free memory. */
int dvm_native_context_release_turn(DVM_NativeContext *context, void *token)
{
    if (dvm_native_context_object_count(context))
    {
        return 0;
    }
    if (!context || tls_get() != context || !token || context->turn != token)
    {
        return 0;
    }
    context->turn = NULL;
    return 1;
}


/* Return only the current owner's reservation; never inspect foreign TLS. */
void *dvm_native_context_turn(DVM_NativeContext *context)
{
    return context && tls_get() == context ? context->turn : NULL;
}


/* Register access is valid only on the context's native owner. */
void *dvm_native_context_registers(DVM_NativeContext *context)
{
    return context && tls_get() == context ? context->registers : 0;
}


/* Borrow the embedding runtime only on the bound native thread. */
void *dvm_native_context_runtime(DVM_NativeContext *context)
{
    return context && tls_get() == context ? context->runtime : 0;
}


/* Borrow the current Dis process without transferring a heap reference. */
void *dvm_native_context_process(DVM_NativeContext *context)
{
    return context && tls_get() == context ? *context->process_slot : 0;
}


/* [GC:] Host must root the process before binding and clear it before release. */
int dvm_native_context_set_process(DVM_NativeContext *context, void *process)
{
    if (!context || tls_get() != context)
    {
        return 0;
    }
    *context->process_slot = process;
    return 1;
}


/* Prevent a typed interpreter adapter from treating undersized storage as REG. */
size_t dvm_native_context_register_size(DVM_NativeContext *context)
{
    return context && tls_get() == context ? context->register_size : 0;
}


/* Install only on the owner; the hook itself coordinates any external readers. */
int dvm_native_context_set_poll(DVM_NativeContext *context, DVM_NativePoll poll, void *user)
{
    if (dvm_native_context_object_count(context))
    {
        return 0;
    }
    if (!context || tls_get() != context || !poll || context->poll)
    {
        return 0;
    }
    context->poll = poll;
    context->poll_user = user;
    return 1;
}


/* Remove only the exact owner's registration, preventing a dangling callback. */
int dvm_native_context_clear_poll(DVM_NativeContext *context, DVM_NativePoll poll, void *user)
{
    if (dvm_native_context_object_count(context))
    {
        return 0;
    }
    if (!context || tls_get() != context || context->poll != poll || context->poll_user != user)
    {
        return 0;
    }
    context->poll = 0;
    context->poll_user = 0;
    return 1;
}


/* No hook preserves the previous TLS-only fixture; no bound context fails closed. */
int dvm_native_context_poll(void)
{
    DVM_NativeContext *context = dvm_native_context_current();
    if (!context || context->object_locks)
    {
        return 0;
    }
    return context->poll ? context->poll(context->poll_user) : 1;
}


/* The typed adapter passes offsetof(ExtendedProc, active) to retain C alignment. */
DVM_NativeContext *dvm_native_context_create_embedded(
    size_t storage_size, size_t register_offset, size_t register_size, size_t process_offset, void *runtime)
{
    DVM_NativeContext *context;
    void *storage;
    if (!storage_size || !register_size || register_offset > storage_size ||
        register_size > storage_size - register_offset || process_offset > storage_size ||
        sizeof(void *) > storage_size - process_offset)
    {
        return 0;
    }
    context = dvm_native_context_create(register_size, runtime);
    if (!context)
    {
        return 0;
    }
    storage = calloc(1, storage_size);
    if (!storage)
    {
        dvm_native_context_destroy(context);
        return 0;
    }
    free(context->storage);
    context->storage = storage;
    context->host_process = storage;
    context->process_slot = (void **)((unsigned char *)storage + process_offset);
    context->registers = (unsigned char *)storage + register_offset;
    return context;
}


/* One TLS owner supplies both up and its embedded active register state. */
void *dvm_native_context_host_process(DVM_NativeContext *context)
{
    return context && tls_get() == context ? context->host_process : 0;
}


/* Borrow only the current context's native heap metadata. No cross-owner read,
 * allocation or heap access; native context destruction ends this lifetime. */
DVM_HeapOwner *dvm_native_owner_current(void)
{
    DVM_NativeContext *context = dvm_native_context_current();
    return context ? &context->heap_owner : NULL;
}


/* Count one successful object-guard acquisition on this bound OS owner. This
 * scalar is native affinity metadata, not a heap root or permission token.
 * Foreign/unbound/overflow refuses before changing any owner state. */
int dvm_native_context_object_acquire(DVM_NativeContext *context)
{
    if (!context || tls_get() != context || context->object_locks == ULONG_MAX)
    {
        return 0;
    }
    context->object_locks++;
    return 1;
}


/* Consume one recorded guard on its bound OS owner; refuse unmatched/foreign
 * calls. The guard implementation separately verifies exact object ownership. */
int dvm_native_context_object_release(DVM_NativeContext *context)
{
    if (!context || tls_get() != context || !context->object_locks)
    {
        return 0;
    }
    context->object_locks--;
    return 1;
}


/* Observe only this bound owner's count. Foreign/unbound returns zero without
 * reading owner-mutated fields; this observation is not a heap access grant. */
unsigned long dvm_native_context_object_count(DVM_NativeContext *context)
{
    return context && tls_get() == context ? context->object_locks : 0;
}


/* Report owner-local hook registration without invoking it or publishing roots.
 * Foreign/unbound returns zero; an installed hook is not itself ACTIVE rights. */
int dvm_native_context_has_poll(DVM_NativeContext *context)
{
    return context && tls_get() == context && context->poll != NULL;
}


/* Register one native reference ledger on current bound context. context is
 * borrowed; return 0 unchanged for foreign/unbound/overflow. No GC roots or
 * heap permissions are granted. Only explicit close consumes registration. */
int dvm_native_context_scope_acquire(DVM_NativeContext *context)
{
    if (!context || tls_get() != context || context->owner_scopes == ULONG_MAX)
    {
        return 0;
    }
    context->owner_scopes++;
    return 1;
}


/* Consume one current-owner ledger registration after all its drops finish.
 * context remains borrowed; return 0 unchanged on foreign/unmatched use. */
int dvm_native_context_scope_release(DVM_NativeContext *context)
{
    if (!context || tls_get() != context || !context->owner_scopes)
    {
        return 0;
    }
    context->owner_scopes--;
    return 1;
}
