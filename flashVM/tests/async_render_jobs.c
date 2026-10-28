/* Deterministic queue concurrency tests: no timing thresholds or sleep-based ordering. */
#include "flashvm/render_queue.h"
#include "channel_sync.h"
#include "thread_port.h"
#include <stdio.h>
#include <stdlib.h>


/* Fixture state survives queue teardown and is protected independently of queue internals. */
typedef struct Fixture
{
    FVM_PortSync *sync;    /* Owned test mutex/condition. */
    unsigned int entered;  /* Number of callbacks parked at the native gate. */
    unsigned int released; /* Number of reclaimed payloads. */
    unsigned int mask;     /* Reclaimed identities; catches double disposal. */
    int open;              /* Native execution gate. */
    int failed;            /* Explicit test invariant failure. */
} Fixture;


/* Each accepted payload is independent except for the explicitly synchronized test gate. */
typedef struct Job
{
    Fixture *fixture; /* Borrowed through queue destruction. */
    unsigned int id;  /* One-based unique identity. */
} Job;


/* Park execution until the owner opens the gate; return a deliberate newest-job error.
 * value borrows Job; all shared fixture access uses its own mutex, never queue state. */
static FVM_Result execute(void *value)
{
    Job *job = (Job *)value;
    Fixture *fixture = job->fixture;
    fvm_port_sync_lock(fixture->sync);
    fixture->entered++;
    fvm_port_sync_signal_all(fixture->sync);
    while (!fixture->open)
    {
        fvm_port_sync_wait(fixture->sync);
    }
    fvm_port_sync_unlock(fixture->sync);
    return job->id == 4 ? FVM_INVALID_ARGUMENT : FVM_OK;
}


/* Count and free one payload on its disposing thread; duplicate release fails explicitly. */
static void release(void *value)
{
    Job *job = (Job *)value;
    Fixture *fixture = job->fixture;
    unsigned int bit = 1U << job->id;
    fvm_port_sync_lock(fixture->sync);
    if (fixture->mask & bit)
    {
        fixture->failed = 1;
    }
    fixture->mask |= bit;
    fixture->released++;
    fvm_port_sync_unlock(fixture->sync);
    free(job);
}


/* Submit a newly allocated native payload, preserving/releasing it on admission failure. */
static int submit(FVM_RenderQueue *queue, Fixture *fixture, unsigned int id)
{
    Job *job = (Job *)malloc(sizeof(*job));
    if (!job)
    {
        return 0;
    }
    job->fixture = fixture;
    job->id = id;
    if (fvm_render_queue_submit(queue, job) != FVM_OK)
    {
        free(job);
        return 0;
    }
    return 1;
}


/* Await a known started callback count using predicates rather than scheduler timing. */
static void await_entered(Fixture *fixture, unsigned int count)
{
    fvm_port_sync_lock(fixture->sync);
    while (fixture->entered < count)
    {
        fvm_port_sync_wait(fixture->sync);
    }
    fvm_port_sync_unlock(fixture->sync);
}


/* Exercise two persistent workers, pending replacement, stale results and failure delivery.
 * No live VM is passed to a callback. Return process failure on any violated ownership rule. */
int main(void)
{
    Fixture fixture;
    FVM_RenderQueue *queue = NULL;
    FVM_RenderQueue *unchanged = NULL;
    void *job = NULL;
    FVM_Result rendered = FVM_EXCEPTION;
    int ok;
    fixture.sync = fvm_port_sync_create();
    fixture.entered = 0;
    fixture.released = 0;
    fixture.mask = 0;
    fixture.open = 0;
    fixture.failed = 0;
    if (!fixture.sync)
    {
        return 1;
    }
    if (fvm_render_queue_create(0, execute, release, &unchanged) != FVM_INVALID_ARGUMENT || unchanged ||
        fvm_render_queue_create(2, execute, release, &queue) != FVM_OK)
    {
        fprintf(stderr, "queue creation contract failed\n");
        fvm_port_sync_destroy(fixture.sync);
        return 1;
    }
    ok = fvm_render_queue_take(queue, &job, &rendered, 1) == FVM_NOT_FOUND && job == NULL &&
         rendered == FVM_EXCEPTION && submit(queue, &fixture, 1);
    if (ok)
    {
        await_entered(&fixture, 1);
        ok = submit(queue, &fixture, 2);
    }
    if (ok)
    {
        await_entered(&fixture, 2);
        /* Both workers remain parked: these calls must finish without raster progress. */
        ok = submit(queue, &fixture, 3) && submit(queue, &fixture, 4) &&
             fvm_render_queue_take(queue, &job, &rendered, 0) == FVM_NOT_FOUND;
        fvm_port_sync_lock(fixture.sync);
        ok = ok && fixture.released == 1 && fixture.mask == (1U << 3);
        fvm_port_sync_unlock(fixture.sync);
    }
    fvm_port_sync_lock(fixture.sync);
    fixture.open = 1;
    fvm_port_sync_signal_all(fixture.sync);
    fvm_port_sync_unlock(fixture.sync);
    if (ok)
    {
        ok = fvm_render_queue_take(queue, &job, &rendered, 1) == FVM_OK;
        if (ok)
        {
            ok = ((Job *)job)->id == 4 && rendered == FVM_INVALID_ARGUMENT;
            release(job);
        }
    }
    fvm_render_queue_destroy(queue);
    ok = ok && !fixture.failed && fixture.released == 4 && fixture.mask == 30 && fixture.entered == 3;
    fvm_port_sync_destroy(fixture.sync);
    fvm_render_queue_destroy(NULL);
    if (!ok)
    {
        fprintf(stderr, "async render queue ownership/latest-result invariant failed\n");
        return 1;
    }
    puts(
        "PASS async render: two parked workers, nonblocking admission, bounded replacement, stale discard, exact release");
    return 0;
}
