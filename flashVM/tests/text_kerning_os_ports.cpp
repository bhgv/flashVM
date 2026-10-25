/* Independent font handles keep different kerning features isolated on real OS workers. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/text_layout.h"
#include "fixtures/font_data.h"
#include "../ports/thread_port.h"
#include <cmath>

#ifdef FVM_HAS_FONTS
struct Job
{
    unsigned flags;
    bool failed;
    unsigned passes;
};


/* Own every mutable font, run and layout; share only immutable original font bytes. */
static void worker(void *raw)
{
    Job *job = (Job *)raw;
    FVM_Font *font = 0;
    FVM_TextLayout *layout = 0;
    FVM_TextRun *run = 0;
    if (fvm_font_read(fvm_test_font, sizeof(fvm_test_font), 0, 40, &font) != FVM_OK)
    {
        job->failed = true;
        return;
    }
    for (unsigned pass = 0; pass < 64; ++pass)
    {
        if (fvm_font_shape_configured(font, "fi", 2, FVM_TEXT_LTR, 0, job->flags, &run) != FVM_OK ||
            fvm_text_run_count(run) != 1)
        {
            job->failed = true;
            break;
        }
        fvm_text_run_destroy(run);
        run = 0;
        if (fvm_text_layout_create_configured(font, "AV", 2, 0, job->flags, &layout) != FVM_OK)
        {
            job->failed = true;
            break;
        }
        FVM_TextLayoutMetrics metrics;
        if (fvm_text_layout_metrics(layout, &metrics) != FVM_OK ||
            std::fabs(metrics.advance_width - (job->flags ? 48 : 44.8)) > 0.02)
        {
            job->failed = true;
            break;
        }
        fvm_text_layout_destroy(layout);
        layout = 0;
        ++job->passes;
    }
    fvm_text_run_destroy(run);
    fvm_text_layout_destroy(layout);
    fvm_font_destroy(font);
}
#endif


/* Join every worker before reading its result; no VM/context/font handle is shared. */
int main()
{
#ifdef FVM_HAS_FONTS
    Job jobs[4];
    FVM_PortThread *threads[4];
    for (unsigned i = 0; i < 4; ++i)
    {
        jobs[i].flags = i % 2 ? FVM_FONT_SHAPE_NO_KERNING : 0;
        jobs[i].failed = false;
        jobs[i].passes = 0;
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
        if (!(!jobs[i].failed && jobs[i].passes == 64))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
#endif
    return 0;
}
