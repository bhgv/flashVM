/* Failed periodic layout construction releases partial paths and leaves the font reusable. */
#define main allocation_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "textfield_counted_text_allocation.cpp"

#undef main
#include <cmath>


int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Font *font = 0;
    if (!(fvm_font_read(fvm_test_font, sizeof(fvm_test_font), 0, 40, &font) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_TextLayoutOptions o = {0, 0, FVM_TEXT_LTR, FVM_TEXT_ALIGN_LEFT, 0};
    double period = 87.5;
    bool success = false;
    unsigned failures = 0;
    for (long boundary = 0; boundary < 2048 && !success; ++boundary)
    {
        FVM_TextLayout *layout = (FVM_TextLayout *)1;
        allowance = boundary;
        FVM_Result result =
            fvm_text_layout_create_periodic(font, "A\tV\tA", 5, &o, FVM_FONT_SHAPE_NO_KERNING, 0, 0, period, &layout);
        allowance = -1;
        if (!(result == FVM_OK || result == FVM_OUT_OF_MEMORY))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        success = result == FVM_OK;
        if (!success)
        {
            ++failures;
            if (!(!layout))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_text_layout_create_periodic(
                      font, "A\tV\tA", 5, &o, FVM_FONT_SHAPE_NO_KERNING, 0, 0, period, &layout) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        FVM_TextLayoutMetrics metrics;
        if (!(fvm_text_layout_metrics(layout, &metrics) == FVM_OK && std::fabs(metrics.advance_width - 199) < 0.02))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_text_layout_destroy(layout);
    }
    if (!(success && failures))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_font_destroy(font);
#endif
    return 0;
}
