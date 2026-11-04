/* Atomic root-group replacement with actual mutex/registry metadata and OS threads. */
#include "root_batch.h"
#include "root_registry.h"
#include "root_registry_private.h"
#include "thread_port.h"
#include "channel_sync.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define OWNERS 8 /* Independent joined OS publishers, not a library quota. */
#define WIDTH 4 /* Occurrences per published native group. */
#define STEPS 2000 /* Replacements per owner,16000 total. */
static __thread int monitoring; /* Only the API interval of this caller. */
static __thread unsigned int locks; /* Actual sync acquisitions in that interval. */
static __thread unsigned int unlocks; /* Actual sync releases in that interval. */
static unsigned int middle; /* Atomic count of publishers paused halfway. */
static unsigned int middle_seen; /* Scanner releases the halfway rendezvous. */
static unsigned int finished; /* Atomic joined-publisher progress for scanner stop. */
static FVM_PortSync *gate; /* Parent-owned start gate, held only during start/join setup. */
static int ready; /* Gate-protected arrival count. */
static int go; /* Gate-protected start flag. */
void *__real_malloc(size_t);
void *__real_calloc(size_t,size_t);
void __real_free(void *);
void __real_fvm_port_sync_lock(FVM_PortSync *);
void __real_fvm_port_sync_unlock(FVM_PortSync *);


/* Keys are actual immutable native records, never fabricated managed Heap data. */
typedef struct Key
{
    unsigned int owner; /* Visitor grouping identity. */
    unsigned int generation; /* Old/new vector identity. */
    unsigned int index; /* Occurrence within a vector. */
} Key;


/* One independent publisher owns its batch; parent retains key storage through join. */
typedef struct Work
{
    Key keys[2][WIDTH]; /* Two live immutable vectors. */
    DVM_RootBatch *batch; /* Preallocated owner-local reservation. */
} Work;


/* Visitor-local aggregate, initialized outside its locked callback interval. */
typedef struct View
{
    unsigned int counts[OWNERS]; /* Occurrences per owner. */
    unsigned int generations[OWNERS]; /* Bitmask of visible vector generations. */
    unsigned int indices[OWNERS]; /* Independent per-owner occurrence coverage. */
} View;


/* Fail at exact semantic10 without allocation or callback under registry lock. */
static void check(int valid,const char *message)
{
    if (!valid)
    {
        const char *prefix = "FAIL root replace: ";
        ssize_t result = write(2,prefix,strlen(prefix));
        (void)result;
        result = write(2,message,strlen(message));
        (void)result;
        result = write(2,"\n",1);
        (void)result;
        _exit(10);
    }
}


/* Reject actual allocator calls inside the publication interval. */
void *__wrap_malloc(size_t size)
{
    check(!monitoring,"replace allocation attempt");
    return __real_malloc(size);
}


/* Reject calloc attempts without replacing the real allocator. */
void *__wrap_calloc(size_t count,size_t size)
{
    check(!monitoring,"replace allocation attempt");
    return __real_calloc(count,size);
}


/* No native metadata or referent may be freed inside replacement. */
void __wrap_free(void *value)
{
    check(!monitoring,"replace allocation attempt");
    __real_free(value);
}


/* Observe actual root-sync intervals without altering exclusion. */
void __wrap_fvm_port_sync_lock(FVM_PortSync *sync)
{
    if (monitoring)
    {
        locks++;
    }
    __real_fvm_port_sync_lock(sync);
}


/* An atomic replacement has exactly one matching actual unlock. */
void __wrap_fvm_port_sync_unlock(FVM_PortSync *sync)
{
    if (monitoring)
    {
        unlocks++;
    }
    __real_fvm_port_sync_unlock(sync);
}


/* Private fixture-only hook injected before relinking. Caller already holds
 * the actual registry mutex. No reentry/allocation: inspect real RootEntry links
 * before a controlled omission could overwrite a still-linked node. */
void dvm_test_before_relink(DVM_RootBatch *batch)
{
    unsigned int bucket;
    for (bucket = 0; bucket < 32; bucket++)
    {
        RootEntry *node = *dvm_root_registry_bucket((void *)((uintptr_t)bucket << 6));
        unsigned int visited = 0;
        while (node)
        {
            check(++visited <= OWNERS*WIDTH+32,"bounded metadata chain");
            check(node->batch != batch,"old roots withdrawn before reuse");
            node = node->next;
        }
    }
}


/* Borrow complete stable vector/batch; require one exclusion interval on success
 * or no lock on input refusal. No managed referent ownership is created. */
static int replace(DVM_RootBatch *batch,void *const *keys,size_t count,int wanted)
{
    int result;
    locks = unlocks = 0;
    monitoring = 1;
    result = dvm_runtime_root_batch_replace(batch,keys,count);
    monitoring = 0;
    check(result == wanted,"invalid vector unchanged");
    check(locks == (unsigned int)wanted && unlocks == locks,"one registry interval");
    return result;
}


/* Visit stable real native keys, recording only local scalar counts. No locks,
 * allocations, referent transfer or registry reentry inside this callback. */
static void visit(void *key,void *argument)
{
    Key *value = key;
    View *view = argument;
    check(value && value->owner < OWNERS && value->generation < 2 && value->index < WIDTH,"visitor native key");
    view->counts[value->owner]++;
    view->generations[value->owner] |= 1U << value->generation;
    view->indices[value->owner] |= 1U << value->index;
}


/* Worker starts together with peers; gate is retained until all joins complete. */
static void start(void)
{
    fvm_port_sync_lock(gate);
    ready++;
    fvm_port_sync_signal_all(gate);
    while (!go)
    {
        fvm_port_sync_wait(gate);
    }
    fvm_port_sync_unlock(gate);
}


/* Own one batch exclusively; retain both native vectors through all operations. */
static void publisher(void *argument)
{
    Work *work = argument;
    unsigned int step;
    start();
    for (step = 0; step < STEPS; step++)
    {
        void *keys[WIDTH];
        unsigned int index;
        for (index = 0; index < WIDTH; index++)
        {
            keys[index] = &work->keys[step&1][index];
        }
        replace(work->batch,keys,WIDTH,1);
        if (step+1 == STEPS/2)
        {
            __atomic_add_fetch(&middle,1,__ATOMIC_RELEASE);
            while (!__atomic_load_n(&middle_seen,__ATOMIC_ACQUIRE))
            {
                fvm_port_thread_sleep_us(100);
            }
        }
    }
    __atomic_add_fetch(&finished,1,__ATOMIC_RELEASE);
}


/* Scan while writers replace independently. Each complete locked visit must
 * see exactly one full generation per owner, never an empty/mixed group. */
static void scanner(void *unused)
{
    unsigned int scans = 0;
    (void)unused;
    start();
    do
    {
        View view = {0};
        unsigned int owner;
        unsigned int paused = __atomic_load_n(&middle,__ATOMIC_ACQUIRE);
        dvm_runtime_roots_visit(visit,&view);
        for (owner = 0; owner < OWNERS; owner++)
        {
            check(view.counts[owner] == WIDTH && (view.generations[owner] == 1 || view.generations[owner] == 2) &&
                view.indices[owner] == (1U << WIDTH)-1,"atomic complete generation");
        }
        if (paused == OWNERS)
        {
            __atomic_store_n(&middle_seen,1,__ATOMIC_RELEASE);
        }
        scans++;
    } while (__atomic_load_n(&finished,__ATOMIC_ACQUIRE) < OWNERS || scans < 100);
}


/* Serial multiset/index/refusal cases precede any concurrency. Keys are native
 * identities only, so controls stop before malformed metadata could reach GC. */
static void serial(void)
{
    Key old[WIDTH];
    Key next[WIDTH];
    void *a[WIDTH];
    void *b[WIDTH];
    void *invalid[WIDTH];
    DVM_RootBatch *batch = dvm_runtime_root_batch_create(WIDTH);
    unsigned int index;
    check(batch != NULL,"reservation");
    for (index = 0; index < WIDTH; index++)
    {
        a[index] = &old[index];
        b[index] = &next[index];
        invalid[index] = b[index];
    }
    replace(NULL,NULL,0,0);
    replace(batch,a,WIDTH,0);
    check(dvm_runtime_root_add(a[0]),"independent ordinary occurrence");
    check(dvm_runtime_root_batch_publish(batch,a,WIDTH),"initial group");
    replace(batch,b,WIDTH,1);
    for (index = 0; index < WIDTH; index++)
    {
        check(dvm_runtime_root_count(a[index]) == (index ? 0UL : 1UL) &&
            dvm_runtime_root_count(b[index]) == 1,"new multiset");
    }
    check(dvm_runtime_root_remove(a[0]),"independent ordinary root preserved");
    check(dvm_runtime_root_batch_remove(batch,3),"new index numbering");
    check(dvm_runtime_root_batch_remove(batch,1),"sparse old group");
    invalid[WIDTH-1] = NULL;
    replace(batch,invalid,WIDTH,0);
    replace(batch,NULL,1,0);
    replace(batch,a,WIDTH+1,0);
    check(dvm_runtime_root_count(b[0]) == 1 && !dvm_runtime_root_count(b[1]) &&
        dvm_runtime_root_count(b[2]) == 1 && !dvm_runtime_root_count(b[3]),"refusal preserves sparse set");
    b[1] = b[0];
    b[3] = b[0];
    replace(batch,b,WIDTH,1);
    check(dvm_runtime_root_count(b[0]) == 3 && dvm_runtime_root_count(b[2]) == 1,"duplicate occurrences");
    check(!dvm_runtime_root_remove(b[0]),"ordinary remove preserves replacement");
    replace(batch,NULL,0,1);
    check(!dvm_runtime_root_count(b[0]) && !dvm_runtime_root_count(b[2]),"active empty replacement");
    check(!dvm_runtime_root_batch_destroy(&batch),"empty remains active");
    replace(batch,a,WIDTH,1);
    check(dvm_runtime_root_batch_clear(batch) && dvm_runtime_root_batch_destroy(&batch) && !batch,"balanced clear/destroy");
}


/* Run real registry/ports only, never a fake Heap/collector. argv may request
 * serial-only semantic controls. All threads and borrowed native keys are joined. */
int main(int argc,char **argv)
{
    Work work[OWNERS];
    FVM_PortThread *threads[OWNERS+1];
    unsigned int owner;
    serial();
    if (argc > 1 && !strcmp(argv[1],"--serial-only"))
    {
        puts("PASS serial root replacement");
        return 0;
    }
    gate = fvm_port_sync_create();
    check(gate != NULL,"thread gate");
    for (owner = 0; owner < OWNERS; owner++)
    {
        void *keys[WIDTH];
        unsigned int index;
        for (index = 0; index < WIDTH; index++)
        {
            unsigned int generation;
            for (generation = 0; generation < 2; generation++)
            {
                work[owner].keys[generation][index].owner = owner;
                work[owner].keys[generation][index].generation = generation;
                work[owner].keys[generation][index].index = index;
            }
            keys[index] = &work[owner].keys[0][index];
        }
        work[owner].batch = dvm_runtime_root_batch_create(WIDTH);
        check(work[owner].batch && dvm_runtime_root_batch_publish(work[owner].batch,keys,WIDTH),"publisher initial roots");
        threads[owner] = fvm_port_thread_start(publisher,&work[owner]);
        check(threads[owner] != NULL,"publisher creation");
    }
    threads[OWNERS] = fvm_port_thread_start(scanner,NULL);
    check(threads[OWNERS] != NULL,"scanner creation");
    fvm_port_sync_lock(gate);
    while (ready != OWNERS+1)
    {
        fvm_port_sync_wait(gate);
    }
    go = 1;
    fvm_port_sync_signal_all(gate);
    fvm_port_sync_unlock(gate);
    for (owner = 0; owner <= OWNERS; owner++)
    {
        fvm_port_thread_join(threads[owner]);
    }
    check(middle == OWNERS && middle_seen,"mid-stress scan before joins");
    for (owner = 0; owner < OWNERS; owner++)
    {
        check(dvm_runtime_root_batch_clear(work[owner].batch) &&
            dvm_runtime_root_batch_destroy(&work[owner].batch),"publisher teardown");
    }
    fvm_port_sync_destroy(gate);
    puts("PASS root replacement:16000 transitions,eight OS publishers,coherent scanner,no allocation");
    return 0;
}
