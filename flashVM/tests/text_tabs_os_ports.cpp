/* Four native font owners copy line geometry before borrowed table mutation on real OS threads. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/text_layout.h"
#include "fixtures/font_data.h"
#include "../ports/thread_port.h"

#ifdef FVM_HAS_FONTS
struct TabsJob
{
    double first;
    unsigned passes;
};


static void tabs_worker(void *raw)
{
    TabsJob *job = (TabsJob *)raw;
    FVM_Font *font = 0;
    if (!(fvm_font_read(fvm_test_font, sizeof(fvm_test_font), 0, 40, &font) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_TextLayoutOptions o = {0, 0, FVM_TEXT_LTR, FVM_TEXT_ALIGN_LEFT, 0};
    for (unsigned i = 0; i < 64; ++i)
    {
        double stops[] = {job->first, job->first * 2};
        FVM_TextLayout *l = 0;
        FVM_TextLayoutMetrics m;
        FVM_TextLayoutGlyph g;
        if (!(fvm_text_layout_create_tabbed(font, "A\tV\tA", 5, &o, FVM_FONT_SHAPE_NO_KERNING, 0.5, 10, stops, 2, &l) ==
              FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        stops[0] = stops[1] = 0;
        if (!(fvm_text_layout_metrics(l, &m) == FVM_OK && m.advance_width == job->first * 2 + 24.5 &&
              fvm_text_layout_glyph(l, 4, &g) == FVM_OK && g.x == job->first * 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Path *p = 0;
        if (!(fvm_text_layout_path(l, &p) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_text_layout_destroy(l);
        if (!(fvm_path_count(p) > 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_path_destroy(p);
        ++job->passes;
    }
    fvm_font_destroy(font);
}
#endif


int main()
{
#ifdef FVM_HAS_FONTS
    TabsJob jobs[4];
    FVM_PortThread *threads[4];
    for (unsigned i = 0; i < 4; ++i)
    {
        jobs[i].first = i % 2 ? 60 : 40;
        jobs[i].passes = 0;
        threads[i] = fvm_port_thread_start(tabs_worker, jobs + i);
        if (!(threads[i]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    for (unsigned i = 0; i < 4; ++i)
    {
        fvm_port_thread_join(threads[i]);
        if (!(jobs[i].passes == 64))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
#endif
    return 0;
}
