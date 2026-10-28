/* FlashVM: Rendering jobs and drawing integration: render jobs.
 * File: src/render/render_jobs.c.
 */

#include "flashvm/render_jobs.h"
#include "../../ports/thread_port.h"
#include <stdlib.h>


/* One immutable partition borrows the caller buffer until its worker is joined. */
typedef struct Worker
{
    FVM_RenderJob callback; /* Borrowed native operation, valid until join. */
    void *userdata;         /* Borrowed caller storage; partitions must not race. */
    unsigned int start;     /* Inclusive native job index. */
    unsigned int end;       /* Exclusive native job index. */
} Worker;


/* Execute the initialized half-open partition in arg; no allocation or ownership transfer.
 * Callback errors cannot be returned through this void batch ABI; callbacks must not throw. */
static void run_worker(void *arg)
{

    Worker *w = (Worker *)arg;
    unsigned int i;
    for (i = w->start; i < w->end; ++i)
    {
        w->callback(w->userdata, i);
    }
}


/* Execute count jobs with up to workers OS threads, joining before return.
 * callback/userdata are borrowed through completion; callbacks own disjoint outputs.
 * Return 0 on success, -1 on invalid input/allocation/launch failure. After partial
 * launch, complete unstarted partitions locally and join all started threads. */
int fvm_render_jobs_run(unsigned int count, unsigned int workers, FVM_RenderJob callback, void *userdata)
{

    Worker *tasks;
    FVM_PortThread **threads;
    unsigned int i;           /* Current initialized partition or launched handle. */
    unsigned int started = 0; /* Handles requiring join. */
    unsigned int n;           /* Nonempty partition count. */
    size_t task_bytes;        /* Checked allocation extent for native partitions. */
    size_t thread_bytes;      /* Checked allocation extent for join handles. */
    int failed = 0;
    if (!callback)
    {
        return -1;
    }
    if (!count)
    {
        return 0;
    }
    if (workers == 0 || count == 1)
    {
        Worker w;
        w.callback = callback;
        w.userdata = userdata;
        w.start = 0;
        w.end = count;
        run_worker(&w);
        return 0;
    }

    n = workers < count ? workers : count;
    task_bytes = (size_t)n * sizeof(*tasks);
    thread_bytes = (size_t)n * sizeof(*threads);
    if (task_bytes / sizeof(*tasks) != n || thread_bytes / sizeof(*threads) != n)
    {
        return -1;
    }
    tasks = (Worker *)malloc(task_bytes);
    threads = (FVM_PortThread **)malloc(thread_bytes);
    if (!tasks || !threads)
    {
        free(tasks);
        free(threads);
        return -1;
    }
    {
        unsigned int cursor = 0;
        /* Initialize every partition before a thread-start failure can enter fallback. */
        for (i = 0; i < n; ++i)
        {
            unsigned int length = count / n + (i < count % n ? 1U : 0U);
            tasks[i].callback = callback;
            tasks[i].userdata = userdata;
            tasks[i].start = cursor;
            cursor += length;
            tasks[i].end = cursor;
        }
    }
    for (i = 0; i < n; ++i)
    {
        threads[i] = fvm_port_thread_start(run_worker, &tasks[i]);
        if (!threads[i])
        {
            failed = 1;
            break;
        }
        ++started;
    }
    /* Complete unstarted partitions synchronously even on partial failure. */
    for (; i < n; ++i)
    {
        run_worker(&tasks[i]);
    }
    for (i = 0; i < started; ++i)
    {
        fvm_port_thread_join(threads[i]);
    }

    free(tasks);
    free(threads);
    return failed ? -1 : 0;
}
