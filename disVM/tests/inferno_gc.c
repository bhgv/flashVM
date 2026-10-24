/* Exercise retained RC/tri-colour GC with explicit synthetic pool backing.
 * The pool is a deterministic test boundary, not a replacement runtime allocator. */
#include "lib9.h"
#ifdef DVM_GC_WORKERS
#include "dat.h"
#include "disvm/gc_workers.h"
#include "disvm/native_process.h"
#endif
#include "isa.h"
#include "interp.h"
#include "pool.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

/* Match real heap headers while controlling allocated/free state and exact counts. */
struct Pool {
    int unused; /* The fixture implements only the pool operations under test. */
};
static Pool test_pool;
Pool *heapmem = &test_pool;
Module *modules;
void (*heapmonitor)(int, void *, ulong);
void (*poolfault)(void *, char *, ulong);
static union { long double alignment; unsigned char bytes[16384]; } arena;
static size_t arena_used;
static unsigned int live_blocks;
static unsigned int freed_blocks;
static Bhdr *arena_header;

/* Allocation diagnostics use a native caller address, not ARM inline assembly. */
uintptr getcallerpc(void *unused)
{
    (void)unused;
    return (uintptr)__builtin_return_address(0);
}

/* Real GC does not print in these cases; keep diagnostics linked without a lib9 SDK. */
int print(char *format, ...)
{
    (void)format;
    return 0;
}
/* Allocate type descriptors exactly as the retained dtype code expects. */
void *mallocz(ulong size, int clear)
{
    return clear ? calloc(1, size) : malloc(size);
}
/* Return the fixture arena chain with actual Bhdr/Btail layouts. */
Bhdr *poolchain(Pool *pool)
{
    assert(pool == heapmem);
    return arena_header;
}
/* Mark a live block freed, preserving its physical extent for the next GC scan. */
void poolfree(Pool *pool, void *pointer)
{
    Bhdr *header;
    assert(pool == heapmem);
    header = (Bhdr *)((unsigned char *)pointer - offsetof(Bhdr, u.data));
    assert(header->magic == MAGIC_A);
    header->magic = MAGIC_F;
    assert(live_blocks);
    live_blocks--;
    freed_blocks++;
}

PRODUCTION_GC
PRODUCTION_HEAP

/* Use native-aligned extents while retaining the actual allocator header format. */
static size_t aligned(size_t size)
{
    size_t alignment = sizeof(void *);
    return (size + alignment - 1) / alignment * alignment;
}
/* Create one test arena; generated functions retain their real GC colour state. */
static void initialize_arena(void)
{
    memset(&arena, 0, sizeof(arena));
    arena_header = (Bhdr *)arena.bytes;
    arena_header->magic = MAGIC_E;
    arena_header->size = aligned(sizeof(Bhdr) + sizeof(Btail));
    arena_header->clink = nil;
    arena_used = arena_header->size;
    arena_header->csize = arena_used;
}
/* Own one real Heap record with its actual Type/map/refcount fields. */
static void *allocate_object(Type *type)
{
    size_t extent = aligned(offsetof(Bhdr, u.data) + sizeof(Heap) + type->size + sizeof(Btail));
    Bhdr *block;
    Heap *heap;
    assert(arena_used + extent <= sizeof(arena.bytes));
    block = (Bhdr *)(arena.bytes + arena_used);
    block->magic = MAGIC_A;
    block->size = extent;
    heap = B2D(block);
    heap->ref = 1;
    heap->color = mutator;
    heap->t = type;
    type->ref++;
    initmem(type, H2D(void *, heap));
    arena_used += extent;
    arena_header->csize = arena_used;
    live_blocks++;
    return H2D(void *, heap);
}
/* Retain an edge using real ref/mark rules; release an existing edge if present. */
static void set_edge(void **slot, void *value)
{
    if (value != H) {
        D2H(value)->ref++;
        Setmark(D2H(value));
    }
    destroy(*slot);
    *slot = value;
}
/* Check allocation state without dereferencing a reclaimed object's payload. */
static int is_live(void *object)
{
    Bhdr *header = (Bhdr *)((unsigned char *)D2H(object) - offsetof(Bhdr, u.data));
    return header->magic == MAGIC_A;
}
#ifdef DVM_GC_WORKERS
/* One rooted deferred release and an incremental-cycle budget; main changes
 * these fields only while both workers are idle. */
typedef struct Backend {
    DVM_NativeWorld *world;
    DVM_GCWorkers *workers;
    Prog *roots;
    void *pending_release;
    size_t cycle_steps;
    DVM_NativeContext *Fast_context;
    DVM_NativeContext *Cycles_context;
} Backend;
static Backend backend;

/* No mutator is registered in this allocator fixture; Prog roots are explicit. */
static void publish_roots(DVM_NativeContext *context, void *registers, size_t size,
                          void *process, void *user)
{
    (void)context; (void)registers; (void)size; (void)process; (void)user;
    assert(0 && "unexpected mutator in the isolated GC allocator fixture");
}
/* [GC:] Removing the pending native root and destroying happen in one stopped phase. */
static DVM_GCWorkResult fast_pass(DVM_NativeContext *context, size_t budget, void *user)
{
    Backend *state = user;
    DVM_WorldState world;
    assert(budget == 1 && dvm_world_state(state->world, &world) && world.stopped);
    state->Fast_context = context;
    if (state->pending_release != H) {
        void *object = state->pending_release;
        state->pending_release = H;
        ptrdel(D2H(object));
        destroy(object);
    }
    return DVM_GC_DONE;
}
/* Advance the real Inferno collector a bounded number of rungc calls. */
static DVM_GCWorkResult cycles_pass(DVM_NativeContext *context, size_t budget, void *user)
{
    Backend *state = user;
    DVM_WorldState world;
    assert(budget == 1 && dvm_world_state(state->world, &world) && world.stopped);
    state->Cycles_context = context;
    if (state->cycle_steps) {
        rungc(state->roots);
        state->cycle_steps--;
    }
    return state->cycle_steps ? DVM_GC_MORE : DVM_GC_DONE;
}
/* Queue ownership is pinned in Inferno's real pointer-root registry. */
static void deferred_release(void *object)
{
    assert(backend.pending_release == H);
    ptradd(D2H(object));
    backend.pending_release = object;
    assert(dvm_gc_workers_request(backend.workers, DVM_GC_FAST));
    assert(dvm_gc_workers_wait_idle(backend.workers));
}
#endif

/* Finish multiple real colour epochs without relying on private colour choices. */
static void cycles(Prog *roots)
{
#ifdef DVM_GC_WORKERS
    backend.roots = roots;
    backend.cycle_steps = 64;
    assert(dvm_gc_workers_request(backend.workers, DVM_GC_CYCLES));
    assert(dvm_gc_workers_wait_idle(backend.workers));
#else
    unsigned int step;
    for (step = 0; step < 64; step++)
        rungc(roots);
#endif
}

/* A mapped link and an unmapped integer deliberately share the same word width. */
typedef struct Record {
    void *edge;
    uintptr_t scalar;
} Record;

/* Verify acyclic RC, unreachable cycles, global roots and integer lookalikes. */
int main(void)
{
    uchar map[] = {0x80};
    Type *record_type;
    Type *module_type;
    Record *first;
    Record *second;
    Record *holder;
    Modlink *module_link;
    Module global_module;
    Prog root;
    unsigned int before;
#ifdef DVM_GC_WORKERS
    DVM_GCWorkersConfig config;
    DVM_GCWorkersState state;
    memset(&config, 0, sizeof(config));
    backend.world = dvm_world_create();
    backend.pending_release = H;
    config.world = backend.world;
    config.create_context = dvm_native_process_create;
    config.roots = publish_roots;
    config.Fast.run = fast_pass;
    config.Fast.budget = 1;
    config.Cycles.run = cycles_pass;
    config.Cycles.budget = 1;
    config.user = &backend;
    backend.workers = dvm_gc_workers_create(&config);
    assert(backend.world && backend.workers);
#endif
    initialize_arena();
    record_type = dtype(freeheap, sizeof(Record), map, sizeof(map));
    module_type = dtype(freeheap, sizeof(Modlink), nil, 0);
    assert(record_type && module_type);
    module_link = allocate_object(module_type);
    module_link->MP = H;
    memset(&root, 0, sizeof(root));
    root.R.M = module_link;
    memset(&global_module, 0, sizeof(global_module));
    global_module.origmp = H;
    modules = &global_module;

    first = allocate_object(record_type);
    second = allocate_object(record_type);
    set_edge(&first->edge, second);
    destroy(second);
    before = freed_blocks;
#ifdef DVM_GC_WORKERS
    deferred_release(first);
#else
    destroy(first);
#endif
    assert(freed_blocks == before + 2);

    first = allocate_object(record_type);
    second = allocate_object(record_type);
    set_edge(&first->edge, second);
    set_edge(&second->edge, first);
    destroy(first);
    destroy(second);
    assert(D2H(first)->ref == 1 && D2H(second)->ref == 1);
    cycles(&root);
    assert(!is_live(first) && !is_live(second));

    holder = allocate_object(record_type);
    first = allocate_object(record_type);
    second = allocate_object(record_type);
    set_edge(&first->edge, second);
    set_edge(&second->edge, first);
    set_edge(&holder->edge, first);
    destroy(first);
    destroy(second);
    global_module.origmp = (uchar *)holder;
    cycles(&root);
    assert(is_live(holder) && is_live(first) && is_live(second));
    holder->scalar = (uintptr_t)first;
    set_edge(&holder->edge, H);
    cycles(&root);
    assert(is_live(holder) && !is_live(first) && !is_live(second));
    global_module.origmp = H;
    destroy(holder);

    /* Locals and parameters share a real typed stack-frame map. */
    {
        typedef struct LocalFrame {
            Frame Frame;     /* Retained frame metadata and return/module links. */
            void *parameter; /* Exactly one strong slot described by frame_map. */
        } LocalFrame;
        uchar frame_map[] = {(uchar)(0x80 >> (offsetof(LocalFrame, parameter) / sizeof(void *)))};
        Type *frame_type = dtype(freeheap, sizeof(LocalFrame), frame_map, sizeof(frame_map));
        Stkext *stack = calloc(1, offsetof(Stkext, reg.tos) + sizeof(LocalFrame));
        LocalFrame *frame;
        assert(frame_type && stack);
        frame = (LocalFrame *)stack->reg.tos.fu;
        frame->Frame.t = frame_type;
        frame->parameter = H;
        root.R.EX = (uchar *)stack;
        root.R.SP = (uchar *)frame + sizeof(*frame);
        first = allocate_object(record_type);
        second = allocate_object(record_type);
        set_edge(&first->edge, second);
        set_edge(&second->edge, first);
        set_edge(&frame->parameter, first);
        destroy(first);
        destroy(second);
        cycles(&root);
        assert(is_live(first) && is_live(second));
        set_edge(&frame->parameter, H);
        cycles(&root);
        assert(!is_live(first) && !is_live(second));
        root.R.EX = nil;
        root.R.SP = nil;
        free(stack);
        freetype(frame_type);
    }
    destroy(module_link);
    freetype(record_type);
    freetype(module_type);
    assert(live_blocks == 0);
#ifdef DVM_GC_WORKERS
    assert(backend.Fast_context && backend.Cycles_context && backend.Fast_context != backend.Cycles_context);
    assert(dvm_gc_workers_state(backend.workers, &state));
    assert(state.Fast.passes == 1 && state.Cycles.passes == 320 && !state.failed);
    assert(dvm_gc_workers_destroy(backend.workers));
    assert(dvm_world_destroy(backend.world));
#endif
    puts("PASS real Inferno destroy/rungc: acyclic RC, cyclic garbage, global/frame roots, unmapped address-valued scalar");
    return 0;
}
