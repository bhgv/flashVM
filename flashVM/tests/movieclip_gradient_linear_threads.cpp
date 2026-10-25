/* Four OS workers retain isolated linearRGB fill/stroke graphs through 128 paint and GC epochs. */
#include <stdio.h>
#include <stdlib.h>
#include "gradient_linear_fixture.hpp"
#include "../ports/thread_port.h"


struct GradientJob
{
    unsigned seed, done;
};


static void gradient_worker(void *raw)
{
    GradientJob *job = (GradientJob *)raw;
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Object *clip = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK &&
          fvm_movieclip_create(c, "worker", 1, &clip) == FVM_OK && fvm_gc_pin(c, clip) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned epoch = 0; epoch < 32; ++epoch)
    {
        const unsigned mode = (job->seed + epoch) % 3;
        const bool radial = (epoch % 2) != 0;
        FVM_Value out;
        if (!(fvm_object_call(c, clip, "clear", 0, 0, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if ((epoch / 2) % 2)
        {
            linear_style(c, clip, mode, radial, true);
            line_gradient_geometry(c, clip);
            linear_pixels(c, clip, mode, radial, true);
        }
        else
        {
            FVM_Value args[8];
            linear_arguments(c, args, mode, radial);
            if (!(fvm_object_call(c, clip, "beginGradientFill", 8, args, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            geometry(c, clip);
            linear_pixels(c, clip, mode, radial, false);
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth))
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
    GradientJob jobs[4];
    FVM_PortThread *threads[4];
    for (unsigned i = 0; i < 4; ++i)
    {
        jobs[i].seed = i;
        jobs[i].done = 0;
        threads[i] = fvm_port_thread_start(gradient_worker, jobs + i);
        if (!(threads[i]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    for (unsigned i = 0; i < 4; ++i)
    {
        fvm_port_thread_join(threads[i]);
        if (!(jobs[i].done == 32))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}
