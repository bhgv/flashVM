/* Four OS workers own independent VMs/fonts/fields and repeatedly scroll, export and collect. */
#include <stdio.h>
#include <stdlib.h>
#include "textfield_scroll_fixture.hpp"
#include "../ports/thread_port.h"

#ifdef FVM_HAS_FONTS
struct ScrollJob
{
    unsigned completed;
};


static void scroll_worker(void *raw)
{
    ScrollJob *job = (ScrollJob *)raw;
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *field = scroll_field(c);
    FVM_PathBounds initial = scroll_bounds(c, field);
    for (unsigned epoch = 0; epoch < 32; ++epoch)
    {
        unsigned top = 1 + epoch % 2, horizontal = epoch % 13;
        scroll_number(c, field, "scroll", top);
        scroll_number(c, field, "hscroll", horizontal);
        FVM_TextFieldScrollInfo i = scroll_info(c, field);
        if (!(i.scroll == top && i.hscroll == horizontal && i.maxscroll == 2 && i.maxhscroll == 12))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_PathBounds b = scroll_bounds(c, field);
        if (!(b.x_min == initial.x_min - horizontal && b.y_min == initial.y_min - (top - 1) * 46))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        ++job->completed;
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
#endif


int main()
{
#ifdef FVM_HAS_FONTS
    ScrollJob jobs[4];
    FVM_PortThread *threads[4];
    for (unsigned i = 0; i < 4; ++i)
    {
        jobs[i].completed = 0;
        threads[i] = fvm_port_thread_start(scroll_worker, jobs + i);
        if (!(threads[i]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    for (unsigned i = 0; i < 4; ++i)
    {
        fvm_port_thread_join(threads[i]);
        if (!(jobs[i].completed == 32))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
#endif
    return 0;
}
