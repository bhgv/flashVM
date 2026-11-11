/* Copyright (c) 2026 bhgv
 * EN: Real Modlink/Module/MP retirement beside admitted peers, with ownership refusals.
 * RU: Настоящее освобождение Modlink/Module/MP при допущенных соседях и отказах владения.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "isa.h"
#include "jit_publication.h"
#include "pool.h"
#include "heap_pool.h"
#include "kernel.h"
#include "runtab-legacy.h"
#include "runtime_gc.h"
#include "managed_owner.h"
#include "root_batch.h"
#include "root_registry.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include "gc/common/runtime_state.h"
#include "thread_port.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char ModuleName[] = "$FlashVM-shared-module-retirement"; /* Unique fixture registry path. */
extern Pool *heapmem;                                                 /* Real heap arena, alive through runtime join. */
static pthread_mutex_t sync = PTHREAD_MUTEX_INITIALIZER;              /* Protect fixture state, not VM fields. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER;             /* Reader and peer notifications. */
static unsigned admitted;                                             /* Peers at native barrier, under sync. */
static unsigned turn;                   /* Sole peer allowed to advance admission epoch, under sync. */
static int released;                    /* Parent completed physical retirement checks, under sync. */
static int expired;                     /* Peer watchdog observed missing parent progress, under sync. */
static Module *target_module;           /* Independently link-owned metadata until final unload. */
static Heap *targets[4];                /* Link, cloned MP, original MP, shared child identities. */
static DVM_HeapKey keys[4];             /* Generation keys distinguish later arena reuse. */
static unsigned expected_mask = 15;     /* Mapped module has four heaps; empty module only its link. */
static unsigned freed;                  /* Exact physical free mask of target allocations, under sync. */
static uchar *code_identity;            /* Native code identity, atomic publication and withdrawal. */
static size_t code_bytes;               /* Immutable native mapping extent through final free. */
static int code_unmapped;               /* Real target munmap completed, under sync. */
static int metadata_freed;              /* Actual free(Module) completed, under sync. */
static unsigned handoffs;               /* Target ticket RC publication count, under sync. */
static int reader_held;                 /* Original ticket reader epoch active, under sync. */
static int reader_release;              /* Parent permits original reader completion, under sync. */
static int reader_wait_seen;            /* Real ticket epoch retry observed, under sync. */
static int later_held;                  /* Separate leaf reader epoch active, under sync. */
static int later_release;               /* Parent permits separate reader completion, under sync. */
static unsigned later_passes;           /* Actual Fast responses while later reader is held. */
static int root_refused;                /* MP root publication refused once, under sync. */
static int claim_refused;               /* Transient last-count claim refused once, under sync. */
static DVM_NativeContext *fast_context; /* Actual target Fast identity through join. */
static DVM_OwnerTransfer ticket;        /* Received exclusively by native observer at thread start. */


int __real_dvm_runtime_guest_sleep(unsigned long);


void __real_Sys_werrstr(void *);


int __real_dvm_runtime_rc_defer(void *);


unsigned int __real_dvm_managed_owner_drain_shared(DVM_NativeContext *, size_t);


unsigned int __real_dvm_runtime_rc_shared_leaves(DVM_NativeContext *, size_t);


int __real_dvm_world_try_quiescent(DVM_NativeWorld *, DVM_NativeContext *);


int __real_dvm_runtime_root_batch_publish(DVM_RootBatch *, void *const *, size_t);


int __real_dvm_refcount_claim_last_u(unsigned long *);


void __real_dvm_gc_fast_module_finalize(Heap *);


int __real_dvm_gc_fast_module_eligible(const Heap *);


void __real_poolfree(Pool *, void *);


void __real_free(void *);


int __real_munmap(void *, size_t);


int __real_cjitexec(REG *, int, int (*)(REG *));

#include "test_module_retirement_relay.inc"


/* [GC:] EN: Observe queued RC ownership before the ticket withdraws its root.
 * The source reference remains live through publication; no inline drop allowed.
 * RU: Наблюдать владение RC до снятия корня ticket. Исходная ссылка живёт весь
 * вызов публикации; снятие вместо передачи не допускается. */
int __wrap_dvm_runtime_rc_defer(void *value)
{
    pthread_mutex_lock(&sync);
    int selected = targets[0] && !(freed & 1) && D2H(value) == targets[0] &&
                   !dvm_native_context_process(dvm_native_context_current());
    if (selected)
    {
        require(!reader_held && later_held, "module RC handoff must follow its ticket epoch");
        fast_context = dvm_native_context_current();
    }
    pthread_mutex_unlock(&sync);
    int result = __real_dvm_runtime_rc_defer(value);
    if (selected)
    {
        pthread_mutex_lock(&sync);
        require(result && dvm_runtime_root_count(targets[0]) == 2, "RC must own the module root before ticket removal");
        handoffs++;
        pthread_mutex_unlock(&sync);
    }
    return result;
}


/* [GC:] EN: Inject one MP-root refusal and inspect rollback before the next
 * publication. batch/keys/count remain caller-owned on refusal.
 * RU: Внедрить отказ корня MP и проверить откат перед следующей публикацией.
 * batch/keys/count при отказе остаются во владении вызывающего. */
int __wrap_dvm_runtime_root_batch_publish(DVM_RootBatch *batch, void *const *values, size_t count)
{
    pthread_mutex_lock(&sync);
    int selected = targets[1] && !(freed & 2) && count == 1 && values[0] == targets[1] &&
                   dvm_native_context_current() == fast_context;
    if (selected)
    {
        require(dvm_runtime_root_count(targets[1]) == 0, "module retry leaked staged data roots");
        require(H2D(Modlink *, targets[0])->MP == H2D(uchar *, targets[1]) &&
                    dvm_refcount_load_u(&targets[0]->ref) == 1,
                "MP refusal must preserve source link and reference");
        if (CONTROL == 2 && !root_refused)
        {
            root_refused = 1;
            pthread_mutex_unlock(&sync);
            return 0;
        }
    }
    pthread_mutex_unlock(&sync);
    return __real_dvm_runtime_root_batch_publish(batch, values, count);
}


/* [GC:] EN: Retain a genuine transient alias across one real last-count claim,
 * then release it before returning refusal. The source MP stays untouched;
 * the real caller must roll back the staged packet before retrying.
 * RU: Удержать настоящий временный алиас через один захват последней ссылки,
 * затем снять его до возврата отказа. MP источника не меняется; настоящий
 * вызывающий должен откатить пакет перед повтором. */
int __wrap_dvm_refcount_claim_last_u(unsigned long *reference)
{
    pthread_mutex_lock(&sync);
    int selected = (CONTROL == 3 || CONTROL == 7) && targets[0] && reference == &targets[0]->ref && !claim_refused;
    if (!selected)
    {
        pthread_mutex_unlock(&sync);
        return __real_dvm_refcount_claim_last_u(reference);
    }
    require(dvm_runtime_root_add(targets[0]) && dvm_refcount_retain_u(reference) == DVM_REFCOUNT_LIVE,
            "independent competing alias acquires root and reference");
    require(!__real_dvm_refcount_claim_last_u(reference), "actual last-count claim observes competing owner");
    require(H2D(Modlink *, targets[0])->MP == H2D(uchar *, targets[1]), "failed claim cannot detach MP");
    require(dvm_runtime_root_remove(targets[0]) && dvm_refcount_drop_nonfinal_u(reference),
            "competing alias releases its own owner");
    claim_refused = 1;
    pthread_mutex_unlock(&sync);
    return 0;
}


/* [GC:] EN: Forward exact eligibility except the disabled-module control.
 * heap remains independently live; no arbitrary finalizer is substituted.
 * RU: Передать точную проверку типа кроме контроля отключения. heap живёт через
 * независимого владельца; произвольный финализатор не подставляется. */
int __wrap_dvm_gc_fast_module_eligible(const Heap *heap)
{
    return CONTROL == 4 ? 0 : __real_dvm_gc_fast_module_eligible(heap);
}


/* [GC:] EN: Verify both cutoffs and MP replacement ownership before known unload.
 * heap remains zero-count/privately claimed until the real finalizer returns.
 * RU: Проверить обе эпохи и замену владельца MP до известного unload. heap
 * остаётся частно захваченным с нулевым счётчиком до возврата финализатора. */
void __wrap_dvm_gc_fast_module_finalize(Heap *heap)
{
    pthread_mutex_lock(&sync);
    if (!(freed & 1) && heap == targets[0])
    {
        require(!reader_held && !later_held, "module finalized before reader epoch elapsed");
        require(H2D(Modlink *, heap)->MP == H && (!targets[1] || dvm_runtime_root_count(targets[1]) == 1),
                "module data lacks its transferred root");
    }
    pthread_mutex_unlock(&sync);
    __real_dvm_gc_fast_module_finalize(heap);
}


/* [GC:] EN: Observe actual Module metadata destruction after registry unlink and
 * origmp publication. value is consumed by real free; do not read it afterwards.
 * RU: Наблюдать настоящее освобождение Module после удаления из реестра и
 * публикации origmp. value потребляется free; после вызова его не читать. */
void __wrap_free(void *value)
{
    Module *identity = __atomic_load_n(&target_module, __ATOMIC_ACQUIRE);
    if (!identity || value != identity)
    {
        __real_free(value);
        return;
    }
    pthread_mutex_lock(&sync);
    int selected = !metadata_freed;
    if (selected)
    {
        DVM_WorldState state;
        require(!reader_held && !later_held && !expired && dvm_world_state(dvm_gc_runtime.world, &state) &&
                    !state.stopping && !state.stopped && state.active >= 2,
                "module metadata destruction must overlap admitted peers");
        require(CONTROL != 9 || code_unmapped, "target JIT mapping must retire before Module metadata");
        require(lookmod((char *)ModuleName) == NULL, "last Module release must unlink before destruction");
        require(!targets[2] || dvm_runtime_root_count(targets[2]) == 1,
                "origmp must enter rooted RC before Module metadata free");
        __atomic_store_n(&target_module, NULL, __ATOMIC_RELEASE);
    }
    pthread_mutex_unlock(&sync);
    __real_free(value);
    if (selected)
    {
        pthread_mutex_lock(&sync);
        metadata_freed = 1;
        pthread_mutex_unlock(&sync);
    }
}


/* [GC:] EN: Observe actual executable mapping release after reader grace.
 * Match the published target identity only, so unrelated system unmaps do not
 * take fixture locks. Withdraw identity before real munmap to permit address reuse.
 * RU: Наблюдать освобождение исполняемой mapping после эпохи. Только целевая
 * идентичность берёт mutex проверки; другие unmap проходят напрямую. Удалить
 * идентичность до настоящего munmap для корректного повторного использования.
 */
int __wrap_munmap(void *address, size_t bytes)
{
    uchar *identity = __atomic_load_n(&code_identity, __ATOMIC_ACQUIRE);
    if (!identity || address != identity)
    {
        return __real_munmap(address, bytes);
    }
    pthread_mutex_lock(&sync);
    DVM_WorldState state;
    require(!reader_held && !later_held && !code_unmapped && bytes == code_bytes &&
                dvm_world_state(dvm_gc_runtime.world, &state) && !state.stopping && !state.stopped && state.active >= 2,
            "JIT mapping destruction must overlap peers after reader grace");
    __atomic_store_n(&code_identity, NULL, __ATOMIC_RELEASE);
    pthread_mutex_unlock(&sync);
    int result = __real_munmap(address, bytes);
    require(result == 0, "actual target code unmap succeeds");
    pthread_mutex_lock(&sync);
    code_unmapped = 1;
    pthread_mutex_unlock(&sync);
    return result;
}


/* [GC:] EN: Count real target frees by allocation generation, rejecting old
 * readers, remaining roots and duplicate target destruction. Publish afterwards.
 * RU: Учесть настоящее освобождение по поколению, запретив живых читателей,
 * оставшиеся корни и повторное уничтожение. Опубликовать результат после free. */
void __wrap_poolfree(Pool *pool, void *value)
{
    pthread_mutex_lock(&sync);
    unsigned index = 0;
    while (index < 4 && (targets[index] != value || !dvm_heap_pool_key_matches(pool, value, keys[index])))
    {
        index++;
    }
    if (index < 4)
    {
        DVM_WorldState state;
        require(!reader_held && !later_held && !expired && !(freed & (1U << index)) &&
                    dvm_world_state(dvm_gc_runtime.world, &state) && !state.stopping && !state.stopped &&
                    state.active >= 2,
                "module heap destruction must overlap admitted peers");
        require(dvm_runtime_root_count(value) == 0, "retiring module heap still has a root");
    }
    __real_poolfree(pool, value);
    if (index < 4)
    {
        freed |= 1U << index;
    }
    pthread_mutex_unlock(&sync);
}

#include "test_module_retirement_case.inc"
