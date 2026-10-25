#include <stdio.h>
#include <stdlib.h>
#include "public_round_scale_fixture.hpp"
#include "../ports/thread_port.h"


struct RoundJob
{
    unsigned axis;
    unsigned done;
};


static void worker(void *arg)
{
    RoundJob *job = (RoundJob *)arg;
    for (int e = -1000; e <= -500; e += 10)
    {
        pr_check(e, job->axis % 3, job->axis > 1);
        ++job->done;
    }
}


int main()
{
    FVM_PortThread *t[4];
    RoundJob jobs[4];
    for (unsigned i = 0; i < 4; ++i)
    {
        jobs[i].axis = i;
        jobs[i].done = 0;
        t[i] = fvm_port_thread_start(worker, jobs + i);
        if (!(t[i]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    for (unsigned i = 0; i < 4; ++i)
    {
        fvm_port_thread_join(t[i]);
        if (!(jobs[i].done == 51))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}
