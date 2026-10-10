#include "flashvm/render_jobs.h"
#include "../../ports/thread_port.h"
#include <stdlib.h>
typedef struct Worker {
    FVM_RenderJob callback;
    void *userdata;
    unsigned int start, end;
} Worker;
static void run_worker(void *arg) {
    Worker *w=(Worker *)arg;
    unsigned int i;
    for (i=w->start;i<w->end;++i) w->callback(w->userdata,i);
}
int fvm_render_jobs_run(unsigned int count, unsigned int workers,
                        FVM_RenderJob callback, void *userdata) {
    Worker *tasks;
    FVM_PortThread **threads;
    unsigned int i, started=0, n;
    int failed=0;
    if (!callback) return -1;
    if (!count) return 0;
    if (workers==0 || count==1) {
        Worker w; w.callback=callback; w.userdata=userdata;
        w.start=0; w.end=count; run_worker(&w); return 0;
    }
    n=workers<count?workers:count;
    tasks=(Worker *)malloc((size_t)n*sizeof(*tasks));
    threads=(FVM_PortThread **)malloc((size_t)n*sizeof(*threads));
    if (!tasks || !threads) { free(tasks); free(threads); return -1; }
    {
    unsigned int cursor=0;
    for (i=0;i<n;++i) {
        unsigned int length=count/n + (i<count%n ? 1U : 0U);
        tasks[i].callback=callback; tasks[i].userdata=userdata;
        tasks[i].start=(unsigned int)((count/n)*i + ((count%n)*i)/n);
        tasks[i].end=(unsigned int)((count/n)*(i+1) + ((count%n)*(i+1))/n);
        threads[i]=fvm_port_thread_start(run_worker,&tasks[i]);
        if (!threads[i]) { failed=1; break; }
        ++started;
    }
    }
    /* Complete unstarted partitions synchronously even on partial failure. */
    for (;i<n;++i) run_worker(&tasks[i]);
    for (i=0;i<started;++i) fvm_port_thread_join(threads[i]);
    free(tasks); free(threads);
    return failed?-1:0;
}
