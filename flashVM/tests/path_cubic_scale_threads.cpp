#include <stdio.h>
#include <stdlib.h>
#include "cubic_scale_fixture.hpp"
#include "../ports/thread_port.h"


struct CubicJob
{
    unsigned done;
};


static void worker(void *arg)
{
    CubicJob *job = (CubicJob *)arg;
    for (unsigned k = 0; k < 7; ++k)
    {
        for (int e = -400; e >= -1000; e -= 100)
        {
            cb_check(k, e, 2, false, true);
            ++job->done;
        }
    }
}


int main()
{
    CubicJob jobs[4];
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
        if (!(jobs[i].done == 49))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}
