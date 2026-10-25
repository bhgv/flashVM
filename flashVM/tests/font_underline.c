/* Analytic CC0 post-table oracle: 1000 units/em, top -75, thickness 50. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/font.h"
#include "fixtures/font_data.h"
#include <math.h>


int main(void)
{
    FVM_FontUnderlineMetrics m;
    if (!(fvm_font_underline_metrics(0, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#ifdef FVM_HAS_FONTS
    {
        FVM_Font *font = 0;
        unsigned i;
        if (!(fvm_font_underline_metrics(0, &m) == FVM_INVALID_ARGUMENT && m.center_y == 0 && m.thickness == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (i = 1; i <= 3; ++i)
        {
            if (!(fvm_font_read(fvm_test_font, sizeof(fvm_test_font), 0, 20 * i, &font) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_font_underline_metrics(font, &m) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fabs(m.center_y - 2 * i) < 0.02 && fabs(m.thickness - i) < 0.02))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_font_destroy(font);
            if (!(fabs(m.thickness - i) < 0.02))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
#else
    m.center_y = 9;
    m.thickness = 8;
    if (!(fvm_font_underline_metrics(0, &m) == FVM_UNSUPPORTED && m.center_y == 0 && m.thickness == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
