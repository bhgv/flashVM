/* Actual hosted error macros/Proc/TLS and native ownership; no managed GC model. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "disvm/native_process.h"
#include "disvm/native_cleanup.h"
#include "disvm/owner_scope.h"
#include "disvm/object_guard.h"
#include "disvm/refcount.h"
#include "thread_port.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define WORKERS 8 /* Actual OS owners with independent Proc error labels. */
#define ROUNDS 40 /* Nested native error/ownership episodes per worker. */

static unsigned long retired = 1; /* Atomic final-free count, not object ownership. */
static const char *expected = "scope unwind probe"; /* Immutable shared diagnostic. */


/* Fail by explicit semantic status10; condition/label borrowed, no recovery.
 * Negative controls must reach this rather than compiler/signal/timeout failure. */
static void check(int condition, const char *label)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL native cleanup predicate: %s\n", label);
        exit(10);
    }
}


/* Real native object; each owner scope or creation owner holds one reference. */
typedef struct Message
{
    unsigned long references; /* Native atomic count, never accessed plainly after publication. */
} Message;


/* Stable state outlives setjmp callbacks; scopes/entries are heap allocated so
 * post-jump observations do not inspect modified indeterminate C automatic locals. */
typedef struct Episode
{
    DVM_OwnerScope outer; /* Open before inner error checkpoint; must survive it. */
    DVM_OwnerScope middle; /* Open after outer checkpoint; removed by rethrow. */
    DVM_OwnerScope inner; /* Open after inner checkpoint; removed by first error. */
    DVM_OwnerEntry outer_entry; /* Native root in outer scope. */
    DVM_OwnerEntry middle_entry; /* Native owner retained across caught inner error. */
    DVM_OwnerEntry inner_entry; /* Native owner cleaned before first jump. */
    int reentry; /* Callback probes global unwind refusal only for the inner object. */
} Episode;



/* Retain already-owned Message. object is borrowed/lifetime-protected; return
 * nonzero for one new owner, false unchanged on invalid native counter. */
static int retain(void *object)
{
    return dvm_refcount_retain_u(&((Message *)object)->references) == DVM_REFCOUNT_LIVE;
}


/* Consume owner, possibly free object. During unwind probe, no new scope/TLS
 * detach/nested error push is permitted. Never touch object after successful RMW. */
static void drop(void *object)
{
    Episode *active = dvm_native_context_runtime(dvm_native_context_current());
    DVM_RefcountResult result;
    if (active && active->reentry)
    {
        DVM_OwnerScope scope = {0};
        check(!dvm_owner_scope_open(&scope), "registration during unwind");
        check(!dvm_native_context_unbind(dvm_native_context_current()), "unbind during unwind");
        check(!dvm_native_error_push(), "error push during unwind");
    }
    result = dvm_refcount_release_u(&((Message *)object)->references);
    check(result != DVM_REFCOUNT_REFUSED, "release native owner");
    if (result == DVM_REFCOUNT_LAST)
    {
        free(object);
        check(dvm_refcount_retain_u(&retired) == DVM_REFCOUNT_LIVE, "final free count");
    }
}

static const DVM_OwnerOps ops = {retain, drop}; /* Immutable callbacks for all threads. */


/* Create object and consume its initial owner after retaining in scope. Scope/
 * entry are stable borrowed owner-local metadata. Failure is semantic/fatal. */
static void fill(DVM_OwnerScope *scope, DVM_OwnerEntry *entry)
{
    Message *message = calloc(1, sizeof(*message));
    check(message != NULL, "allocation");
    message->references = 1;
    check(dvm_owner_scope_open(scope), "scope open");
    check(dvm_owner_scope_retain(scope, entry, message, &ops), "scope retain");
    drop(message);
}

PRODUCTION_ERROR_FUNCTIONS


/* Real protected callback opens inner scope then raises through production
 * error/nexterror. argument borrows Episode. Return is unreachable by contract. */
static int inner(void *argument)
{
    Episode *episode = argument;
    fill(&episode->inner, &episode->inner_entry);
    episode->reentry = 1;
    error((char *)expected);
    return 1;
}


/* Preserve middle owner through inner catch, then rethrow to its caller. scope
 * storage is heap-resident, borrowed via argument; outer predates both frames. */
static int middle(void *argument)
{
    Episode *episode = argument;
    fill(&episode->middle, &episode->middle_entry);
    check(!kprotect(inner, episode), "inner error caught");
    episode->reentry = 0;
    check(!episode->inner.context && !episode->inner_entry.object, "inner cleanup before jump");
    check(episode->middle.context && episode->middle_entry.object, "checkpoint preserves middle owner");
    check(episode->outer.context && episode->outer_entry.object, "checkpoint preserves outer owner");
    check(strcmp(up->env->errstr, expected) == 0, "diagnostic preserved");
    error((char *)expected);
    return 1;
}


/* Consume one generic native cleanup record, unlink then free its own storage.
 * argument owns the record. Dispatcher must not read it after this returns. */
static int free_record(void *argument)
{
    DVM_NativeCleanup *record = argument;
    check(dvm_native_cleanup_unlink(record), "unlink before freeing record");
    free(record);
    return 1;
}


/* Each owner runs real nested jumps, normal-pop refusal and guard refusal.
 * argument unused; no managed roots/heap or native JIT is simulated. */
static void worker(void *argument)
{
    Episode *episode = calloc(1, sizeof(*episode));
    DVM_NativeContext *context = dvm_native_process_create(episode);
    DVM_NativeCleanupMark mark = {0};
    DVM_ObjectDomain *domain;
    DVM_ObjectGuard guard = {0};
    char error_text[ERRMAX] = {0};
    int index;
    (void)argument;
    check(context && dvm_native_context_bind(context), "bind extended up");
    up->env = &up->defenv;
    up->env->errstr = error_text;
    for (index = 0; index < ROUNDS; index++)
    {
        check(episode != NULL, "episode allocation");
        memset(episode, 0, sizeof(*episode));
        fill(&episode->outer, &episode->outer_entry);
        check(!kprotect(middle, episode), "outer error caught");
        check(!episode->middle.context && !episode->middle_entry.object, "middle cleanup on rethrow");
        check(episode->outer.context && episode->outer_entry.object, "outer survives rethrow");
        check(up->nerr == 0, "independent balanced native error stack");
        check(dvm_owner_scope_close(&episode->outer), "outer final cleanup");

    }
    check(dvm_native_cleanup_mark(&mark), "empty checkpoint");
    domain = dvm_object_domain_create();
    check(domain && dvm_object_guard_init(&guard, domain), "guard setup");
    check(dvm_object_guard_try_enter(&guard) == DVM_OBJECT_ACQUIRED, "guard enter");
    check(!dvm_native_cleanup_unwind(&mark), "guarded unwind refuses");
    check(dvm_object_guard_leave(&guard), "guard leave");
    check(dvm_object_guard_fini(&guard) && dvm_object_domain_destroy(domain), "guard retire");
    {
        DVM_OwnerScope scope = {0};
        check(dvm_native_error_push(), "normal explicit push");
        check(dvm_owner_scope_open(&scope), "normal pending scope");
        check(!dvm_native_error_pop() && up->nerr == 1, "normal pop refuses pending scope");
        check(dvm_owner_scope_close(&scope), "normal explicit close");
        check(dvm_native_error_pop() && up->nerr == 0, "normal pop after close");
        check(!dvm_native_error_pop() && !dvm_native_error_unwind(), "empty error stack refuses");
        check(dvm_native_cleanup_mark(&mark), "before closed checkpoint scope");
        check(dvm_owner_scope_open(&scope) && dvm_owner_scope_close(&scope), "scope closes before unwind");
        check(dvm_native_cleanup_unwind(&mark), "generation survives closed records");
        check(dvm_owner_scope_open(&scope), "scope before captured generation");
        check(dvm_native_cleanup_mark(&mark), "capture live scope generation");
        check(dvm_owner_scope_close(&scope), "close captured scope");
        check(dvm_owner_scope_open(&scope), "reuse same scope address");
        check(dvm_native_cleanup_unwind(&mark) && !scope.context, "generation rather than stale scope pointer");
        check(dvm_native_cleanup_mark(&mark), "generic cleanup mark");
        {
            DVM_NativeCleanup *record = calloc(1, sizeof(*record));
            check(record && dvm_native_cleanup_link(record, free_record, record), "self-retiring native record");
            check(dvm_native_cleanup_unwind(&mark), "dispatcher avoids retired record");
        }
        up->nerr = NERR;
        check(!dvm_native_error_push(), "error stack overflow refuses");
        up->nerr = 0;
        mark.serial = ULONG_MAX;
        check(!dvm_native_cleanup_unwind(&mark), "future checkpoint refuses");
    }
    check(dvm_native_context_unbind(context) && dvm_native_context_destroy(context), "owner retirement");
    free(episode);
}


/* Launch independent real native owners and verify every allocated message is
 * freed exactly once. No arguments; return0 only after complete join/retirement. */
int main(void)
{
    FVM_PortThread *threads[WORKERS];
    int index;
    for (index = 0; index < WORKERS; index++)
    {
        threads[index] = fvm_port_thread_start(worker, NULL);
        check(threads[index] != NULL, "OS thread creation");
    }
    for (index = 0; index < WORKERS; index++)
    {
        fvm_port_thread_join(threads[index]);
    }
    check(dvm_refcount_load_u(&retired) == 1 + WORKERS * ROUNDS * 3, "exact native final frees");
    puts("PASS native cleanup: 8 OS owners, 640 nested error transfers, 960 native frees, guard/normal-pop fences");
    return 0;
}
