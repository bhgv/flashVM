/* Guard source retain and destination publication; retire displaced owner later. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "raise.h"
#include "shared_pointer.h"
#include "shared_access.h"
#include "shared_arithmetic.h"
#include "runtime_gc.h"
#include "isa.h"
#include "disvm/native_process.h"
#include "managed_owner.h"
#include "gc_fields.h"
#include "root_registry_private.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include <stdint.h>
#include <stdlib.h>


unsigned long dvm_runtime_parallel_movp_module_commits;
unsigned long dvm_runtime_parallel_movp_commits; /* Atomic completed shared pointer moves. */
int dvm_runtime_parallel_movp; /* Startup-only restricted frame-pointer experiment. */


/* Geometric span only; caller independently supplies mapped/rooted storage. */
static int span(const void *cell)
{
    uintptr_t address = (uintptr_t)cell;
    return address >= sizeof(void *) && address <= (uintptr_t)-1 - sizeof(void *);
}


/* Private previous cannot overwrite either managed cell or its partial overlap. */
static int overlaps(const void *one, const void *two)
{
    uintptr_t a = (uintptr_t)one;
    uintptr_t b = (uintptr_t)two;
    return a < b + sizeof(void *) && b < a + sizeof(void *);
}




/* [PERF MOVP-088] Measured guarded-copy cost: PERFORMANCE-COSTS.md. No allocation
 * in initialized cell begin/end; separate barrier/guard/tail costs are unmeasured. */


/* Link a caller-stack occurrence under the registry mutex. The cell guard
 * already owns the displaced reference and NativeWorld covers its lifetime. */
static void root_displaced(RootEntry *entry, FVM_PortSync *mutex, void *value)
{
    RootEntry **head = dvm_root_registry_bucket(D2H(value));
    fvm_port_sync_lock(mutex);
    entry->key = D2H(value);
    entry->batch = entry;
    entry->next = *head;
    *head = entry;
    fvm_port_sync_unlock(mutex);
}


/* Withdraw exactly this occurrence before handing its owner to destroy.
 * No callback, allocation or collector handoff occurs during unlink. */
static void unroot_displaced(RootEntry *entry, FVM_PortSync *mutex)
{
    RootEntry **link = dvm_root_registry_bucket(entry->key);
    fvm_port_sync_lock(mutex);
    while (*link && *link != entry)
    {
        link = &(*link)->next;
    }
    if (*link != entry)
    {
        abort();
    }
    *link = entry->next;
    fvm_port_sync_unlock(mutex);
}


/* [GC:] Caller protects cell lifetime. Capture unaligned values only after all
 * required guards are held. No VM callback, throwing allocation or destruction.
 * Refusal leaves destination/output unchanged and undoes a failed mark retain. */
static const char *movp_attempt(const void *source, void *destination,
    void **previous, DVM_CellResult *claim, RootEntry *root, FVM_PortSync *root_mutex, int selected, DVM_CellDomain *shared_domain)
{
    DVM_CellAccess access = {0};
    const char *problem = NULL;
    void *incoming;
    void *outgoing;
    Heap *heap = NULL;

    *claim = DVM_CELL_REFUSED;
    if (!selected || !span(source) || !span(destination) || !span(previous) ||
        overlaps(previous, source) || overlaps(previous, destination))
    {
        return exHeap;
    }
    if (selected == 2)
    {
        *claim = shared_domain ? dvm_cell_access_begin(shared_domain, source, destination, &access) :
            dvm_managed_cells_begin(source, destination, &access);
        if (*claim != DVM_CELL_ACQUIRED)
        {
            return exHeap;
        }
    }
    memmove(&incoming, source, sizeof(incoming));
    if (incoming == nil)
    {
        problem = exNilref;
    }
    else if (incoming != H)
    {
        if ((uintptr)incoming <= sizeof(Heap) || (uintptr)incoming % sizeof(void *) != 0)
        {
            problem = exHeap;
        }
        else
        {
            heap = D2H(incoming);
            if (!dvm_refcount_load_u(&heap->ref) ||
                dvm_refcount_load_u(&heap->ref) == (ulong)-1)
            {
                problem = exHeap;
            }
        }
    }
    if (!problem)
    {
        memmove(&outgoing, destination, sizeof(outgoing));
        if (outgoing != H && ((uintptr)outgoing <= sizeof(Heap) ||
            (uintptr)outgoing % sizeof(void *) != 0 ||
            !dvm_refcount_load_u(&D2H(outgoing)->ref)))
        {
            problem = exHeap;
        }
    }
    if (!problem && heap)
    {
        if (dvm_refcount_retain_u(&heap->ref) != DVM_REFCOUNT_LIVE)
        {
            problem = exHeap;
        }
        else if (!dvm_gc_field_mark(&heap->color, &mutator, &nprop))
        {
            if (dvm_refcount_release_u(&heap->ref) != DVM_REFCOUNT_LIVE)
            {
                abort();
            }
            problem = exHeap;
        }
    }
    if (!problem && shared_domain && incoming == outgoing)
    {
        /* The destination still owns the same reference. Drop the temporary
         * retain under the guard without registering a displaced owner. */
        if (heap && dvm_refcount_release_u(&heap->ref) != DVM_REFCOUNT_LIVE)
        {
            abort();
        }
        outgoing = H;
    }
    else if (!problem)
    {
        if (root && outgoing != H)
        {
            root_displaced(root, root_mutex, outgoing);
        }
        memmove(destination, &incoming, sizeof(incoming));
    }
    if (selected == 2 && !dvm_cell_access_end(&access))
    {
        abort();
    }
    if (!problem)
    {
        *previous = outgoing;
    }
    return problem;
}



/* Copy a borrowed reference and return its displaced owner on success. */
const char *dvm_runtime_movp_copy(const void *source, void *destination, void **previous)
{
    DVM_CellResult claim;

    return movp_attempt(source, destination, previous, &claim, NULL, NULL, dvm_runtime_shared_mode(), NULL);
}


/* Execute a decoded move and retire its displaced owner after publication.
 * The stack root spans publication and unlock. It never survives a VM error. */
static const char *movp_active_opcode(int opcode, const void *source, void *destination, int *replay)
{
    RootEntry root = {0};
    FVM_PortSync *mutex = NULL;
    DVM_CellResult claim;
    void *previous;
    const char *problem;
    int mode = dvm_runtime_shared_mode();
    DVM_CellDomain *domain = NULL;
    int released = 0;
    int module_operand = 0;

    *replay = 0;
    if (!mode)
    {
        return "pointer move requires managed dispatch";
    }
    if (mode == 2)
    {
        mutex = dvm_root_registry_sync(1);
        if (!mutex)
        {
            return exNomem;
        }
    }
    if (mode == 2 && dvm_runtime_parallel_movp && opcode == IMOVP && up->prog &&
        dvm_native_execution()->arithmetic_retry_opcode == opcode)
    {
        int source_module = dvm_runtime_module_pointer(source);
        int destination_module = dvm_runtime_module_pointer(destination);
        module_operand = source_module || destination_module;
        if ((source_module || dvm_runtime_frame_pointer(source)) &&
            (destination_module || dvm_runtime_frame_pointer(destination)))
        {
            domain = dvm_managed_cell_domain();
            if (domain)
            {
                released = dvm_runtime_gc_shared_try_enter();
            }
        }
    }
    problem = movp_attempt(source, destination, &previous, &claim, mutex ? &root : NULL,
        mutex, mode, released > 0 ? domain : NULL);
    if (released > 0)
    {
        dvm_runtime_gc_shared_finish();
        if (!problem)
        {
            __atomic_fetch_add(&dvm_runtime_parallel_movp_commits, 1, __ATOMIC_RELAXED);
            if (module_operand)
                __atomic_fetch_add(&dvm_runtime_parallel_movp_module_commits, 1, __ATOMIC_RELAXED);
        }
    }
    *replay = claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(opcode);
    if (released > 0 && claim == DVM_CELL_BUSY && !*replay)
    {
        problem = movp_attempt(source, destination, &previous, &claim, &root,
            mutex, mode, NULL);
    }
    if (*replay || problem)
    {
        return *replay ? NULL : problem;
    }
    if (root.key)
    {
        unroot_displaced(&root, mutex);
    }
    destroy(previous);
    return NULL;
}


/* EN: Record either an untouched fallback or a displaced, rooted reference.
 * RU: Хранить данные резервного пути либо вытесненную ссылку с корнем. */
typedef struct MovpDispatch
{
    int opcode;           /* Decoded operation used for retry policy. */
    const void *source;   /* Borrowed rooted source cell. */
    void *destination;    /* Borrowed rooted destination cell. */
    int *replay;          /* Caller-owned retry result. */
    const char *problem;  /* Error returned by the admitted implementation. */
    RootEntry *root;      /* Stack root after a committed shared move. */
    FVM_PortSync *mutex;   /* Protects the displaced root list link. */
    void *previous;       /* Displaced reference, or H if no owner to retire. */
} MovpDispatch;


/* EN: Finish a legacy copy or retire the old reference after shared publication.
 * RU: Выполнить старое копирование или снять ссылку после публикации в SHARED. */
static int complete_movp_dispatch(void *argument)
{
    MovpDispatch *move = argument;
    if (move->root)
    {
        unroot_displaced(move->root, move->mutex);
        destroy(move->previous);
    }
    else
    {
        move->problem = movp_active_opcode(move->opcode, move->source, move->destination, move->replay);
    }
    return 1;
}


/* EN: Admit only the fallback or old-owner destruction, then restore SHARED.
 * RU: Допустить только резервный путь или освобождение старой ссылки. */
static const char *finish_movp_dispatch(MovpDispatch *move)
{
    dvm_runtime_gc_dispatch_finish();
    int completed = kprotect(complete_movp_dispatch, move);
    dvm_runtime_gc_dispatch_restore();
    if (!completed)
    {
        krethrow();
    }
    return move->problem;
}


/* EN: Copy proven direct operands in the caller's existing SHARED dispatch.
 * RU: Копировать проверенные прямые операнды в текущем SHARED. */
const char *dvm_runtime_movp_opcode(int opcode, const void *source, void *destination, int *replay)
{
    *replay = 0;
    if (!dvm_runtime_gc_dispatch_is_shared())
    {
        return movp_active_opcode(opcode, source, destination, replay);
    }

    MovpDispatch move = {opcode, source, destination, replay, NULL, NULL, NULL, H};
    int source_module = dvm_runtime_module_pointer(source);
    int destination_module = dvm_runtime_module_pointer(destination);
    DVM_CellDomain *domain = dvm_managed_dispatch_cell_domain();
    if (opcode != IMOVP || !domain ||
        (!source_module && !dvm_runtime_frame_pointer(source)) ||
        (!destination_module && !dvm_runtime_frame_pointer(destination)))
    {
        return finish_movp_dispatch(&move);
    }
    move.mutex = dvm_root_registry_sync(1);
    if (!move.mutex)
    {
        return exNomem;
    }
    RootEntry root = {0};
    DVM_CellResult claim;
    const char *problem = movp_attempt(source, destination, &move.previous, &claim,
        &root, move.mutex, 2, domain);
    if (claim == DVM_CELL_BUSY)
    {
        return finish_movp_dispatch(&move);
    }
    if (problem)
    {
        return problem == exHeap ? "invalid shared pointer move" : problem;
    }
    __atomic_fetch_add(&dvm_runtime_parallel_movp_commits, 1, __ATOMIC_RELAXED);
    if (source_module || destination_module)
    {
        __atomic_fetch_add(&dvm_runtime_parallel_movp_module_commits, 1, __ATOMIC_RELAXED);
    }
    if (root.key)
    {
        move.root = &root;
        return finish_movp_dispatch(&move);
    }
    return NULL;
}
