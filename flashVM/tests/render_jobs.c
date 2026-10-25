#include <stdio.h>
#include <stdlib.h>
#include "flashvm/render_jobs.h"

#define COUNT 257


static void render(void *data, unsigned int index)
{
    unsigned int *outputs = (unsigned int *)data;
    outputs[index] = index + 1;
}


int main(void)
{
    unsigned int outputs[COUNT];
    unsigned int i;
    for (i = 0; i < COUNT; ++i)
    {
        outputs[i] = 0;
    }
    if (!(fvm_render_jobs_run(COUNT, 4, render, outputs) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < COUNT; ++i)
    {
        if (!(outputs[i] == i + 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_render_jobs_run(COUNT, 0, render, outputs) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_render_jobs_run(0, 4, render, outputs) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_render_jobs_run(1, 1, 0, outputs) == -1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
