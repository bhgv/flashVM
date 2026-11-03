/* Actual range guards on native cells; no managed Heap/root is synthesized. */
#include "disvm/cell_access.h"
#include "disvm/refcount.h"
#include "thread_port.h"
#include "channel_sync.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define THREADS 8 /* Joined native TLS owners, not a scheduler or thread quota. */
#define ROUNDS 2000 /* Protected paired writes per native owner. */

void dvm_test_cell_domain_faults(void); /* Separate actual allocator-ledger fixture. */
void dvm_test_cell_noalloc_start(void); /* Constructor-only ledger before worker creation. */
void dvm_test_cell_noalloc_end(void); /* Require zero allocator attempts in begin/end. */


/* Pointer alignment plus enough live bytes to choose known modulo256 stripes. */
typedef union Storage
{
    void *alignment; /* Force pointer-aligned start without type-punning payload. */
    unsigned char bytes[544 * sizeof(void *)]; /* Mapped native storage, alive through joins. */
} Storage;


/* All shared startup/finish predicates use native sync; payload uses cell guards. */
typedef struct Fixture
{
    DVM_CellDomain *domain; /* Creator owns through every access and join. */
    FVM_PortSync *sync; /* Native handshake, never a managed callback. */
    unsigned char *first; /* Unaligned native counter, canonical stripes16/17. */
    unsigned char *second; /* Disjoint native counter, canonical stripes19/20. */
    DVM_CellAccess *foreign; /* Main's acquired storage; worker may only test refusal. */
    int ready; /* Prober arrival, protected by sync. */
    int go; /* Main permits bounded try-only contention checks. */
    int done; /* Prober completed; main polls without a native wait while guarded. */
} Fixture;


/* Explicit semantic10; unsafe continuation and timing failures are not controls. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprintf(stderr, "FAIL cell access: %s\n", message);
        exit(10);
    }
}


/* Bind only bare native TLS; no managed admission/root/payload authority. */
static DVM_NativeContext *bind(void)
{
    DVM_NativeContext *context = dvm_native_context_create(sizeof(long), NULL);
    check(context && dvm_native_context_bind(context), "bind native owner");
    return context;
}


/* Retire exact context after every guarded access has ended. */
static void unbind(DVM_NativeContext *context)
{
    check(dvm_native_context_unbind(context) && dvm_native_context_destroy(context), "retire native owner");
}


/* Fixture-only zero predicate; caller owns local metadata or completed thread. */
static int zero(DVM_CellAccess *access)
{
    unsigned int index;
    if (access->domain || access->context || access->count || access->held)
    {
        return 0;
    }
    for (index = 0; index < 4; index++)
    {
        if (access->guards[index])
        {
            return 0;
        }
    }
    return 1;
}


/* Try while main holds16/17. Claim free14 first, then fail16 and unwind14.
 * Main waits for completion by bounded metadata polling, not guard-owner wait.
 * Native counter storage/domain stay independently alive throughout. */
static void probe(void *argument)
{
    Fixture *fixture = argument;
    DVM_NativeContext *context = bind();
    DVM_CellAccess access = {0};
    fvm_port_sync_lock(fixture->sync);
    fixture->ready = 1;
    fvm_port_sync_signal_all(fixture->sync);
    while (!fixture->go)
    {
        fvm_port_sync_wait(fixture->sync);
    }
    fvm_port_sync_unlock(fixture->sync);
    check(!dvm_cell_access_end(fixture->foreign), "foreign end refuses unchanged");
    check(dvm_cell_access_begin(fixture->domain, fixture->first - 2 * sizeof(void *) - 1,
        fixture->first + 1, &access) == DVM_CELL_BUSY && zero(&access) &&
        !dvm_native_context_object_count(context), "partial busy rollback");
    check(dvm_cell_access_begin(fixture->domain, fixture->first - 2 * sizeof(void *) - 1,
        fixture->first - 2 * sizeof(void *) - 1, &access) == DVM_CELL_ACQUIRED,
        "lower stripe released on busy");
    check(dvm_cell_access_end(&access), "lower access ends");
    fvm_port_sync_lock(fixture->sync);
    fixture->done = 1;
    fvm_port_sync_unlock(fixture->sync);
    unbind(context);
}


/* Reversed argument order still acquires one sorted set. Pair increments are
 * native byte copies under range guards; they never forge a managed object. */
static void write_pair(void *argument)
{
    Fixture *fixture = argument;
    DVM_NativeContext *context = bind();
    unsigned int index;
    for (index = 0; index < ROUNDS; index++)
    {
        DVM_CellAccess access = {0};
        DVM_CellResult result;
        uintptr_t first;
        uintptr_t second;
        void *source = index % 2 ? fixture->second : fixture->first;
        void *destination = index % 2 ? fixture->first : fixture->second;
        while ((result = dvm_cell_access_begin(fixture->domain, source, destination, &access)) == DVM_CELL_BUSY)
        {
            check(zero(&access), "busy access zero before native retry");
            fvm_port_thread_sleep_us(1);
        }
        check(result == DVM_CELL_ACQUIRED, "native pair claim");
        memmove(&first, fixture->first, sizeof(first));
        memmove(&second, fixture->second, sizeof(second));
        check(second == 3 * first, "paired native invariant");
        first++;
        second += 3;
        memmove(fixture->first, &first, sizeof(first));
        memmove(fixture->second, &second, sizeof(second));
        check(dvm_cell_access_end(&access) && zero(&access), "native pair release clears metadata");
    }
    unbind(context);
}


/* Serial controls precede native competitors. Address geometry deliberately
 * exercises unaligned/overlapping/wrapping stripe cases on actual mapped bytes. */
int main(void)
{
    Storage storage = {0};
    Fixture fixture = {0};
    DVM_NativeContext *context;
    DVM_CellAccess access = {0};
    DVM_ObjectDomain *extra_domain;
    DVM_ObjectGuard extra = {0};
    FVM_PortThread *prober;
    FVM_PortThread *threads[THREADS];
    uintptr_t address = (uintptr_t)storage.bytes;
    uintptr_t first;
    uintptr_t second;
    unsigned int index;
    int complete = 0;
    dvm_test_cell_domain_faults();
    fixture.domain = dvm_cell_domain_create();
    check(fixture.domain != NULL, "domain creates");
    check(dvm_cell_access_begin(fixture.domain, (void *)1, (void *)1, &access) == DVM_CELL_REFUSED &&
        zero(&access), "unbound refusal before cells");
    context = bind();
    fixture.first = storage.bytes + (256 + (16 + 256 - (address / sizeof(void *)) % 256) % 256) * sizeof(void *) + 1;
    fixture.second = fixture.first + 3 * sizeof(void *);
    check(sizeof(uintptr_t) == sizeof(void *) && fixture.first > storage.bytes + 2 * sizeof(void *), "fixture geometry");
    check(dvm_cell_access_begin(fixture.domain, NULL, fixture.first, &access) == DVM_CELL_REFUSED &&
        dvm_cell_access_begin(fixture.domain, (void *)(uintptr_t)-1, fixture.first, &access) == DVM_CELL_REFUSED &&
        zero(&access), "invalid spans refuse unchanged");
    extra_domain = dvm_object_domain_create();
    check(extra_domain && dvm_object_guard_init(&extra, extra_domain), "independent extra guard");
    dvm_test_cell_noalloc_start();
    check(dvm_cell_access_begin(fixture.domain, fixture.first, fixture.first + 1, &access) == DVM_CELL_ACQUIRED &&
        access.count == 2, "overlap covers both boundary cells");
    for (index = 1; index < access.count; index++)
    {
        check((uintptr_t)access.guards[index - 1] < (uintptr_t)access.guards[index], "ascending unique guard order");
    }
    check(!dvm_cell_domain_destroy(&fixture.domain) && fixture.domain, "live access refuses domain retirement");
    check(dvm_object_guard_try_enter(&extra) == DVM_OBJECT_ACQUIRED &&
        !dvm_cell_access_end(&access) && access.held == 2, "extra guard prevents premature end");
    check(dvm_object_guard_leave(&extra), "extra guard release before end");
    check(dvm_cell_access_end(&access) && zero(&access), "serial end clears metadata");
    dvm_test_cell_noalloc_end();
    check(dvm_object_guard_fini(&extra) && dvm_object_domain_destroy(extra_domain), "extra domain retires");
    fixture.sync = fvm_port_sync_create();
    check(fixture.sync != NULL, "native probe gate");
    fixture.foreign = &access;
    prober = fvm_port_thread_start(probe, &fixture);
    check(prober != NULL, "prober starts");
    fvm_port_sync_lock(fixture.sync);
    while (!fixture.ready)
    {
        fvm_port_sync_wait(fixture.sync);
    }
    fvm_port_sync_unlock(fixture.sync);
    check(dvm_cell_access_begin(fixture.domain, fixture.first, fixture.first, &access) == DVM_CELL_ACQUIRED,
        "main owns overlapping boundary cells");
    fvm_port_sync_lock(fixture.sync);
    fixture.go = 1;
    fvm_port_sync_signal_all(fixture.sync);
    fvm_port_sync_unlock(fixture.sync);
    for (index = 0; index < 100000000 && !complete; index++)
    {
        fvm_port_sync_lock(fixture.sync);
        complete = fixture.done;
        fvm_port_sync_unlock(fixture.sync);
    }
    check(complete && access.held == 2 && dvm_native_context_object_count(context) == 2,
        "foreign contention preserves current owner");
    check(dvm_cell_access_end(&access) && zero(&access), "main releases before join");
    fvm_port_thread_join(prober);
    for (index = 0; index < THREADS; index++)
    {
        threads[index] = fvm_port_thread_start(write_pair, &fixture);
        check(threads[index] != NULL, "pair writer starts");
    }
    for (index = 0; index < THREADS; index++)
    {
        fvm_port_thread_join(threads[index]);
    }
    memmove(&first, fixture.first, sizeof(first));
    memmove(&second, fixture.second, sizeof(second));
    check(first == THREADS * ROUNDS && second == 3 * first, "joined exact pair values");
    check(dvm_cell_domain_destroy(&fixture.domain) && !fixture.domain, "complete domain retirement");
    fvm_port_sync_destroy(fixture.sync);
    unbind(context);
    puts("PASS cell access:8 OS writers,16000 unaligned paired updates, overlap/foreign/partial-busy cleanup");
    return 0;
}
