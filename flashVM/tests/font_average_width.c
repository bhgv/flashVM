/* Independent fixture OS/2 xAvgCharWidth=547, unitsPerEm=1000; scaled 26.6 rounding. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/font.h"
#include "fixtures/font_data.h"


int main(void)
{
    double width = 123;
    FVM_Font *font = 0;
    if (!(fvm_font_average_width(0, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#ifdef FVM_HAS_FONTS
    if (!(fvm_font_average_width(0, &width) == FVM_INVALID_ARGUMENT && width == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_font_read(fvm_test_font, sizeof(fvm_test_font), 0, 40, &font) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_font_average_width(font, &width) == FVM_OK && width == 21.875))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_font_destroy(font);
    if (!(fvm_font_read(fvm_test_font, sizeof(fvm_test_font), 0, 20, &font) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_font_average_width(font, &width) == FVM_OK && width == 10.9375))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_font_destroy(font);
#else
    if (!(fvm_font_average_width(font, &width) == FVM_UNSUPPORTED && width == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
