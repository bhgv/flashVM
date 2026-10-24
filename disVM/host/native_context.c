/* Real OS TLS and FlashVM mutexes: no green-thread scheduler in this layer. */
#include "disvm/native_context.h"
#include "channel_sync.h"
#include <stdlib.h>
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
struct DVM_NativeContext {
    FVM_PortSync *sync;    /* Protect exclusive bind/unbind ownership. */
    void *registers;       /* Owned zeroed storage, aligned by calloc. */
    size_t register_size; /* Exact extent checked by the typed REG adapter. */
    void *runtime;        /* Borrowed for the complete context lifetime. */
    void *process;        /* Owner-thread process reference; cleared on unbind. */
    int bound;            /* Nonzero while one native TLS slot owns this context. */
};

#ifdef _WIN32
static INIT_ONCE tls_once = INIT_ONCE_STATIC_INIT;
static DWORD tls_key = TLS_OUT_OF_INDEXES;
/* Initialize one process-lifetime Windows TLS slot; never silently share R. */
static BOOL CALLBACK initialize_tls(PINIT_ONCE once, PVOID parameter, PVOID *result)
{
    (void)once; (void)parameter; (void)result;
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
        return 0;
    context = (DVM_NativeContext *)calloc(1, sizeof(*context));
    if (!context)
        return 0;
    context->sync = fvm_port_sync_create();
    context->registers = calloc(1, register_size);
    context->runtime = runtime;
    context->register_size = register_size;
    if (!context->sync || !context->registers) {
        if (context->sync)
            fvm_port_sync_destroy(context->sync);
        free(context->registers);
        free(context);
        return 0;
    }
    return context;
}

/* Caller must quiesce users before destroy; the bound-state check prevents misuse. */
int dvm_native_context_destroy(DVM_NativeContext *context)
{
    if (!context)
        return 1;
    fvm_port_sync_lock(context->sync);
    if (context->bound) {
        fvm_port_sync_unlock(context->sync);
        return 0;
    }
    fvm_port_sync_unlock(context->sync);
    fvm_port_sync_destroy(context->sync);
    free(context->registers);
    free(context);
    return 1;
}

/* Reserve one context for one native thread, rolling back a failed TLS publish. */
int dvm_native_context_bind(DVM_NativeContext *context)
{
    int ok;
    if (!context || !tls_ready() || tls_get())
        return 0;
    fvm_port_sync_lock(context->sync);
    if (context->bound) {
        fvm_port_sync_unlock(context->sync);
        return 0;
    }
    ok = tls_set(context);
    if (ok)
        context->bound = 1;
    fvm_port_sync_unlock(context->sync);
    return ok;
}

/* Detach while still owning the context; another native thread cannot unbind it. */
int dvm_native_context_unbind(DVM_NativeContext *context)
{
    if (!context || tls_get() != context)
        return 0;
    fvm_port_sync_lock(context->sync);
    if (!tls_set(0)) {
        fvm_port_sync_unlock(context->sync);
        return 0;
    }
    context->process = 0;
    context->bound = 0;
    fvm_port_sync_unlock(context->sync);
    return 1;
}

/* Never infer ownership from a global current-process variable. */
DVM_NativeContext *dvm_native_context_current(void)
{
    return (DVM_NativeContext *)tls_get();
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
    return context && tls_get() == context ? context->process : 0;
}
/* [GC:] Host must root the process before binding and clear it before release. */
int dvm_native_context_set_process(DVM_NativeContext *context, void *process)
{
    if (!context || tls_get() != context)
        return 0;
    context->process = process;
    return 1;
}

/* Prevent a typed interpreter adapter from treating undersized storage as REG. */
size_t dvm_native_context_register_size(DVM_NativeContext *context)
{
    return context && tls_get() == context ? context->register_size : 0;
}
