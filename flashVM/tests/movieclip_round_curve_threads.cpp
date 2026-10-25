#include <stdio.h>
#include <stdlib.h>
#include "round_curve_fixture.hpp"
#include "../ports/thread_port.h"


struct CurveJob
{
    unsigned done;
};


static void worker(void *arg)
{
    CurveJob *job = (CurveJob *)arg;
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ss_context(rt, &c);
    for (unsigned n = 0; n < 24; ++n)
    {
        FVM_Object *clip = rc_clip(c, 4, n % 4);
        ss_xy(c, clip, "moveTo", 100, 100);
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        bounds_equal(rb_bounds(c, clip), 0, 2, 16, 10);
        if (!(!c->gc_native_depth && !c->gc_frames))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_unpin(c, clip) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        ++job->done;
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}


int main()
{
    CurveJob jobs[4];
    FVM_PortThread *threads[4];
    for (unsigned i = 0; i < 4; ++i)
    {
        jobs[i].done = 0;
        threads[i] = fvm_port_thread_start(worker, jobs + i);
        if (!(threads[i]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    for (unsigned i = 0; i < 4; ++i)
    {
        fvm_port_thread_join(threads[i]);
        if (!(jobs[i].done == 24))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}
