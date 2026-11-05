/* Exercise retained RC/tri-colour GC with explicit synthetic pool backing.
 * The pool is a deterministic test boundary, not a replacement runtime allocator. */
#include <stdlib.h>
#include "lib9.h"

#ifdef DVM_GC_WORKERS
#include <pthread.h>
#include <sched.h>
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
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>


/* Match real heap headers while controlling allocated/free state and exact counts. */
struct Pool
{
    int unused; /* The fixture implements only the pool operations under test. */
};

static Pool test_pool;
Pool *heapmem = &test_pool;
Module *modules;
void (*heapmonitor)(int, void *, ulong);
void (*poolfault)(void *, char *, ulong);

static union
{
    long double alignment;
    unsigned char bytes[16384];
} arena;

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
    if (!(pool == heapmem))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return arena_header;
}


/* Mark a live block freed, preserving its physical extent for the next GC scan. */
void poolfree(Pool *pool, void *pointer)
{
    Bhdr *header;
    if (!(pool == heapmem))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    header = (Bhdr *)((unsigned char *)pointer - offsetof(Bhdr, u.data));
    if (!(header->magic == MAGIC_A))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    header->magic = MAGIC_F;
    if (!(live_blocks))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
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
    if (!(arena_used + extent <= sizeof(arena.bytes)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
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
    if (value != H)
    {
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
typedef struct Backend
{
    DVM_NativeWorld *world;
    DVM_GCWorkers *workers;
    Prog *roots;
    void *pending_release;
    size_t cycle_steps;
    DVM_NativeContext *Fast_context;
    DVM_NativeContext *Cycles_context;
    unsigned int graph_snapshots; /* Count stopped register publications. */
} Backend;

static Backend backend;


/* Capture the typed process snapshot of an active graph mutator. The world
 * owns all callback arguments through this stopped visit. */
static void publish_roots(DVM_NativeContext *context, void *registers, size_t size, void *process, void *user)
{
    Backend *state = user;
    Prog *guest = process;
    (void)context;
    if (!guest || !registers || size != sizeof(REG))
    {
        fprintf(stderr, "invalid stopped graph mutator snapshot\n");
        exit(EXIT_FAILURE);
    }
    memmove(&guest->R, registers, sizeof(guest->R));
    state->graph_snapshots++;
}


/* [GC:] Removing the pending native root and destroying happen in one stopped phase. */
static DVM_GCWorkResult fast_pass(DVM_NativeContext *context, size_t budget, void *user)
{
    Backend *state = user;
    DVM_WorldState world;
    if (!(budget == 1 && dvm_world_state(state->world, &world) && world.stopped))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    state->Fast_context = context;
    if (state->pending_release != H)
    {
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
    if (!(budget == 1 && dvm_world_state(state->world, &world) && world.stopped))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    state->Cycles_context = context;
    if (state->cycle_steps)
    {
        rungc(state->roots);
        state->cycle_steps--;
    }
    return state->cycle_steps ? DVM_GC_MORE : DVM_GC_DONE;
}


/* Queue ownership is pinned in Inferno's real pointer-root registry. */
static void deferred_release(void *object)
{
    if (!(backend.pending_release == H))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ptradd(D2H(object));
    backend.pending_release = object;
    if (!(dvm_gc_workers_request(backend.workers, DVM_GC_FAST)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_gc_workers_wait_idle(backend.workers)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}
#endif


/* Finish multiple real colour epochs without relying on private colour choices. */
static void cycles(Prog *roots)
{
#ifdef DVM_GC_WORKERS
    backend.roots = roots;
    backend.cycle_steps = 64;
    if (!(dvm_gc_workers_request(backend.workers, DVM_GC_CYCLES)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_gc_workers_wait_idle(backend.workers)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#else
    unsigned int step;
    for (step = 0; step < 64; step++)
    {
        rungc(roots);
    }
#endif
}


/* A mapped link and an unmapped integer deliberately share the same word width. */
typedef struct Record
{
    void *edge;
    uintptr_t scalar;
} Record;


#ifdef DVM_GC_WORKERS
typedef struct GraphRun
{
    pthread_mutex_t mutex; /* Protects phase and completion counters only. */
    int phase; /* 0 holds entry, 1 publishes edges, 2 removes edges, 3 exits. */
    int entered; /* Both mutators must be active before phase 1. */
    int changed[2]; /* Number of completed writes in each phase. */
} GraphRun;

typedef struct GraphMutator
{
    GraphRun *run; /* Borrowed until join. */
    void **slot; /* One disjoint mapped slot in the common rooted record. */
    Record *cycle; /* Borrowed disjoint cycle until the first stopped pass. */
    Prog guest; /* Owner-local snapshot storage until detach. */
} GraphMutator;


/* Stop the fixture with a named failed requirement. */
static void graph_check(int valid, const char *message)
{
    if (!valid)
    {
        fprintf(stderr, "parallel GC graph: %s\n", message);
        exit(EXIT_FAILURE);
    }
}


/* Run two graph changes on a native world participant. Allocation and final
 * destruction stay with the stopped collector or coordinator. */
static void *mutate_graph(void *argument)
{
    GraphMutator *worker = argument;
    GraphRun *run = worker->run;
    DVM_NativeContext *context = dvm_native_process_create(NULL);
    DVM_WorldParticipant *participant;
    int completed = 0;
    graph_check(context && dvm_native_context_bind(context), "bind worker");
    participant = dvm_world_join(backend.world, context);
    ((REG *)dvm_native_context_registers(context))->M = worker->guest.R.M;
    graph_check(participant && dvm_native_context_set_process(context, &worker->guest) &&
        dvm_world_enter(participant), "enter parallel world");
    pthread_mutex_lock(&run->mutex);
    run->entered++;
    pthread_mutex_unlock(&run->mutex);
    for (;;)
    {
        int phase;
        pthread_mutex_lock(&run->mutex);
        phase = run->phase;
        pthread_mutex_unlock(&run->mutex);
        if (phase == 3)
        {
            break;
        }
        if (phase > completed)
        {
            set_edge(worker->slot, phase == 1 ? worker->cycle : H);
            completed = phase;
            pthread_mutex_lock(&run->mutex);
            run->changed[phase-1]++;
            pthread_mutex_unlock(&run->mutex);
        }
        graph_check(dvm_native_context_poll(), "cooperative collector poll");
        sched_yield();
    }
    graph_check(dvm_world_leave(participant) && dvm_world_detach(participant) &&
        dvm_native_context_unbind(context) && dvm_native_context_destroy(context),
        "detach completed worker");
    return NULL;
}


/* Wait for both owner-local changes without changing collector admission. */
static void wait_graph_counter(GraphRun *run, const int *counter)
{
    int done;
    do
    {
        pthread_mutex_lock(&run->mutex);
        done = *counter == 2;
        pthread_mutex_unlock(&run->mutex);
        if (!done)
        {
            sched_yield();
        }
    } while (!done);
}


/* Verify one common root with two independently mutated cyclic branches,
 * stopped snapshots while both mutators are active, then exact reclamation. */
static void parallel_graph(Type *record_type, Prog *roots)
{
    uchar map[] = {0xc0};
    Type *pair_type = dtype(freeheap, 2*sizeof(void *), map, sizeof(map));
    GraphRun run = {PTHREAD_MUTEX_INITIALIZER, 0, 0, {0, 0}};
    GraphMutator workers[2] = {{0}};
    pthread_t threads[2];
    Record *cycles_by_worker[2][2];
    void **pair = allocate_object(pair_type);
    DVM_WorldState world;
    unsigned int snapshots = backend.graph_snapshots;
    unsigned int freed = freed_blocks;
    int phase;
    int index;
    ptradd(D2H(pair));
    for (index = 0; index < 2; index++)
    {
        Record *left = allocate_object(record_type);
        Record *right = allocate_object(record_type);
        set_edge(&left->edge, right);
        set_edge(&right->edge, left);
        destroy(left);
        destroy(right);
        cycles_by_worker[index][0] = left;
        cycles_by_worker[index][1] = right;
        workers[index].run = &run;
        workers[index].slot = &pair[index];
        workers[index].cycle = left;
        workers[index].guest.exval = H;
        workers[index].guest.R.M = roots->R.M;
        graph_check(!pthread_create(&threads[index], NULL, mutate_graph, &workers[index]),
            "create graph mutator");
    }
    workers[0].guest.next = &workers[1].guest;
    workers[1].guest.next = roots;
    wait_graph_counter(&run, &run.entered);
    graph_check(dvm_world_state(backend.world, &world) && world.active == 2,
        "two active OS mutators");
    for (phase = 1; phase <= 2; phase++)
    {
        pthread_mutex_lock(&run.mutex);
        run.phase = phase;
        pthread_mutex_unlock(&run.mutex);
        wait_graph_counter(&run, &run.changed[phase-1]);
        cycles(&workers[0].guest);
        graph_check(backend.graph_snapshots == snapshots + 128 && is_live(pair), "visit both active roots");
        snapshots = backend.graph_snapshots;
        for (index = 0; index < 2; index++)
        {
            graph_check(is_live(cycles_by_worker[index][0]) == (phase == 1) &&
                is_live(cycles_by_worker[index][1]) == (phase == 1), "trace and reclaim cyclic branch");
        }
        graph_check(freed_blocks == freed + (phase == 2 ? 4 : 0), "exact cycle retirement");
    }
    pthread_mutex_lock(&run.mutex);
    run.phase = 3;
    pthread_mutex_unlock(&run.mutex);
    for (index = 0; index < 2; index++)
    {
        graph_check(!pthread_join(threads[index], NULL), "join graph mutator");
    }
    pthread_mutex_destroy(&run.mutex);
    ptrdel(D2H(pair));
    deferred_release(pair);
    graph_check(freed_blocks == freed + 5 && !is_live(pair), "Fast retires remaining owner once");
    freetype(pair_type);
    puts("PASS two active graph mutators: stopped roots, Cycles and Fast retirement");
}
#endif


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
    if (!(backend.world && backend.workers))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    initialize_arena();
    record_type = dtype(freeheap, sizeof(Record), map, sizeof(map));
    module_type = dtype(freeheap, sizeof(Modlink), nil, 0);
    if (!(record_type && module_type))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    module_link = allocate_object(module_type);
    module_link->MP = H;
    memset(&root, 0, sizeof(root));
    root.R.M = module_link;
    root.exval = H;
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
    if (!(freed_blocks == before + 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }

    first = allocate_object(record_type);
    second = allocate_object(record_type);
    set_edge(&first->edge, second);
    set_edge(&second->edge, first);
    destroy(first);
    destroy(second);
    if (!(D2H(first)->ref == 1 && D2H(second)->ref == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    cycles(&root);
    if (!(!is_live(first) && !is_live(second)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }

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
    if (!(is_live(holder) && is_live(first) && is_live(second)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    holder->scalar = (uintptr_t)first;
    set_edge(&holder->edge, H);
    cycles(&root);
    if (!(is_live(holder) && !is_live(first) && !is_live(second)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    global_module.origmp = H;
    destroy(holder);

    /* Locals and parameters share a real typed stack-frame map. */
    {
        typedef struct LocalFrame
        {
            Frame Frame;     /* Retained frame metadata and return/module links. */
            void *parameter; /* Exactly one strong slot described by frame_map. */
        } LocalFrame;

        uchar frame_map[] = {(uchar)(0x80 >> (offsetof(LocalFrame, parameter) / sizeof(void *)))};
        Type *frame_type = dtype(freeheap, sizeof(LocalFrame), frame_map, sizeof(frame_map));
        Stkext *stack = calloc(1, offsetof(Stkext, reg.tos) + sizeof(LocalFrame));
        LocalFrame *frame;
        if (!(frame_type && stack))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
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
        if (!(is_live(first) && is_live(second)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        set_edge(&frame->parameter, H);
        cycles(&root);
        if (!(!is_live(first) && !is_live(second)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        root.R.EX = nil;
        root.R.SP = nil;
        free(stack);
        freetype(frame_type);
    }
#ifdef DVM_GC_WORKERS
    parallel_graph(record_type, &root);
#endif
    destroy(module_link);
    freetype(record_type);
    freetype(module_type);
    if (!(live_blocks == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#ifdef DVM_GC_WORKERS
    if (!(backend.Fast_context && backend.Cycles_context && backend.Fast_context != backend.Cycles_context))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_gc_workers_state(backend.workers, &state)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(state.Fast.passes == 2 && state.Cycles.passes == 448 && !state.failed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_gc_workers_destroy(backend.workers)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_world_destroy(backend.world)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    puts(
        "PASS real Inferno destroy/rungc: acyclic RC, cyclic garbage, global/frame roots, unmapped address-valued scalar");
    return 0;
}
