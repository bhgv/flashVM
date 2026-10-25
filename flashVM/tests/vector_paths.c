#include <stdio.h>
#include <stdlib.h>
#include "flashvm/path.h"
#include <math.h>
#include <string.h>


static void add(FVM_Path *p, FVM_PathVerb verb, double x0, double y0, double x1, double y1, double x2, double y2)
{
    FVM_PathCommand c;
    memset(&c, 0, sizeof(c));
    c.verb = verb;
    c.x[0] = x0;
    c.y[0] = y0;
    c.x[1] = x1;
    c.y[1] = y1;
    c.x[2] = x2;
    c.y[2] = y2;
    if (!(fvm_path_add(p, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void rectangle(FVM_Path *p, double lo, double hi)
{
    add(p, FVM_PATH_MOVE, lo, lo, 0, 0, 0, 0);
    add(p, FVM_PATH_LINE, hi, lo, 0, 0, 0, 0);
    add(p, FVM_PATH_LINE, hi, hi, 0, 0, 0, 0);
    add(p, FVM_PATH_LINE, lo, hi, 0, 0, 0, 0);
    add(p, FVM_PATH_CLOSE, 0, 0, 0, 0, 0, 0);
}


int main(void)
{
    FVM_Path *p = 0, *copy = 0;
    FVM_PathBounds bounds;
    FVM_PathCommand command, before;
    FVM_Affine shear = {1, 0, 0.5, -1, 10, 20}, huge = {1000000, 0, 0, 1000000, 0, 0};
    unsigned char pixels[16 * 72], source[16 * 64];
    FVM_RasterSurface surface = {0, 16, 16, 72}, image = {0, 16, 16, 64};
    FVM_RasterColor red = {255, 0, 0, 255}, green = {0, 255, 0, 255};
    unsigned int x, y;
    surface.pixels = pixels;
    image.pixels = source;
    if (!(fvm_path_create(0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_path_create(&p) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_path_bounds(p, &bounds) == FVM_OK && bounds.x_min == 0 && bounds.y_max == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(&command, 0, sizeof(command));
    command.verb = FVM_PATH_LINE;
    if (!(fvm_path_add(p, &command) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    add(p, FVM_PATH_MOVE, 0, 0, 0, 0, 0, 0);
    add(p, FVM_PATH_QUADRATIC, 10, 10, 20, 0, 0, 0);
    if (!(fvm_path_bounds(p, &bounds) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(bounds.x_max == 20 && fabs(bounds.y_max - 5) < 1e-10))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_path_set_point(p, 1, 0, 10, 20) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_path_bounds(p, &bounds) == FVM_OK && fabs(bounds.y_max - 10) < 1e-10))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_path_set_point(p, 1, 2, 0, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_path_create(&copy) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_path_append(copy, p, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_path_transform(copy, &shear) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_path_get(p, 1, &before) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(before.x[0] == 10 && before.y[0] == 20))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_path_get(copy, 1, &command) == FVM_OK && command.x[0] == 30 && command.y[0] == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_path_transform(p, &huge) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_path_get(p, 1, &command) == FVM_OK && memcmp(&command, &before, sizeof(command)) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_path_destroy(p);
    fvm_path_destroy(copy);
    if (!(fvm_path_create(&p) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    add(p, FVM_PATH_MOVE, 0, 0, 0, 0, 0, 0);
    add(p, FVM_PATH_CUBIC, 0, 10, 10, 10, 10, 0);
    if (!(fvm_path_bounds(p, &bounds) == FVM_OK && fabs(bounds.y_max - 7.5) < 1e-10))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_path_destroy(p);
    if (!(fvm_path_create(&p) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    rectangle(p, 2, 14);
    rectangle(p, 6, 10);
    memset(pixels, 0, sizeof(pixels));
    for (y = 0; y < 16; ++y)
    {
        for (x = 64; x < 72; ++x)
        {
            pixels[y * 72 + x] = 0x5a;
        }
    }
#if defined(FVM_HAS_AGG) || defined(FVM_HAS_CAIRO)
    if (!(fvm_path_draw(p, &surface, red, 0, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(pixels[4 * 72 + 4 * 4] == 255 && pixels[4 * 72 + 4 * 4 + 3] == 255))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(pixels[8 * 72 + 8 * 4 + 3] == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    } /* Same winding but even/odd hole. */
    if (!(fvm_path_draw(p, &surface, green, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(pixels[8 * 72 + 8 * 4 + 1] == 255 && pixels[8 * 72 + 8 * 4 + 3] == 255))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(pixels, 0, sizeof(pixels));
    if (!(fvm_path_draw(p, &surface, red, 2, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(pixels[4 * 72 + 2 * 4 + 3] > 0 && pixels[4 * 72 + 4 * 4 + 3] == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(pixels, 0, sizeof(pixels));
    for (y = 0; y < 16; ++y)
    {
        for (x = 0; x < 16; ++x)
        {
            source[y * 64 + x * 4] = 20;
            source[y * 64 + x * 4 + 1] = 40;
            source[y * 64 + x * 4 + 2] = 80;
            source[y * 64 + x * 4 + 3] = 128;
        }
    }
    if (!(fvm_path_mask(p, &surface, &image, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(pixels[4 * 72 + 4 * 4] == 20 && pixels[4 * 72 + 4 * 4 + 3] == 128))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(pixels[8 * 72 + 8 * 4 + 3] == 0 && pixels[0] == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_path_mask(p, &surface, &surface, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(pixels[4 * 72 + 4 * 4] == 20 && pixels[4 * 72 + 4 * 4 + 3] > 128))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (y = 0; y < 16; ++y)
    {
        for (x = 64; x < 72; ++x)
        {
            pixels[y * 72 + x] = 0x5a;
        }
    }
    if (!(fvm_path_draw(p, &surface, red, 0, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (y = 0; y < 16; ++y)
    {
        for (x = 64; x < 72; ++x)
        {
            if (!(pixels[y * 72 + x] == 0x5a))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
#else
    if (!(fvm_path_draw(p, &surface, red, 0, 0) == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_path_mask(p, &surface, &image, 0) == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    (void)green;
#endif
    if (!(fvm_path_draw(p, &surface, red, -1, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_path_draw(p, &surface, red, 0, 2) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    surface.stride = 63;
    if (!(fvm_path_draw(p, &surface, red, 0, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_path_destroy(p);
    fvm_path_destroy(0);
    return 0;
}
