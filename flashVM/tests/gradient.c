#include "flashvm/gradient.h"
#include <assert.h>
#include <string.h>
#include <math.h>
static FVM_Path *box(void)
{
    FVM_Path *p = 0; FVM_PathCommand command; int i;
    const double xy[] = {0, 0, 16, 0, 16, 16, 0, 16};
    assert(fvm_path_create(&p) == FVM_OK); memset(&command, 0, sizeof(command));
    for (i = 0; i < 8; i += 2) { command.verb = i ? FVM_PATH_LINE : FVM_PATH_MOVE; command.x[0] = xy[i]; command.y[0] = xy[i + 1]; assert(fvm_path_add(p, &command) == FVM_OK); }
    command.verb = FVM_PATH_CLOSE; assert(fvm_path_add(p, &command) == FVM_OK); return p;
}
static void stop(FVM_GradientStop *s, double ratio, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    s->ratio = ratio; s->color.r = r; s->color.g = g; s->color.b = b; s->color.a = a;
}
static int near(unsigned char actual, unsigned int expected) { return (unsigned int)actual + 1 >= expected && actual <= expected + 1; }
int main(void)
{
    FVM_Path *path = box(); FVM_Gradient gradient, invalid; FVM_GradientStop stops[4]; FVM_RasterSurface surface;
    unsigned char pixels[16 * 16 * 4], original[sizeof(pixels)], *p; FVM_Result r; unsigned int i;
    assert(fvm_gradient_init(0) == FVM_INVALID_ARGUMENT); assert(fvm_gradient_init(&gradient) == FVM_OK);
    gradient.matrix.xx = gradient.matrix.yy = 8; gradient.matrix.x0 = gradient.matrix.y0 = 8;
    stop(stops, 0, 255, 0, 0, 255); stop(stops + 1, 1, 0, 0, 255, 255); gradient.stops = stops; gradient.count = 2;
    surface.pixels = pixels; surface.width = surface.height = 16; surface.stride = 64; memset(pixels, 0, sizeof(pixels));
    r = fvm_path_draw_gradient(path, &surface, &gradient, 0); assert(r == FVM_OK || r == FVM_UNSUPPORTED);
    if (r == FVM_OK) {
        p = pixels + 4 * (8 * 16); assert(near(p[0], 247) && p[1] == 0 && near(p[2], 8) && p[3] == 255);
        gradient.matrix.xx = -8; memset(pixels, 0, sizeof(pixels)); assert(fvm_path_draw_gradient(path, &surface, &gradient, 1) == FVM_OK);
        p = pixels + 4 * (8 * 16); assert(near(p[0], 8) && near(p[2], 247));
        gradient.matrix.xx = gradient.matrix.yy = 0; gradient.matrix.xy = -8; gradient.matrix.yx = 8;
        memset(pixels, 0, sizeof(pixels)); assert(fvm_path_draw_gradient(path, &surface, &gradient, 0) == FVM_OK);
        p = pixels + 4 * 8; assert(near(p[0], 247) && near(p[2], 8));
        gradient.matrix.xx = gradient.matrix.yy = 8; gradient.matrix.xy = gradient.matrix.yx = 0; gradient.type = FVM_GRADIENT_RADIAL;
        memset(pixels, 0, sizeof(pixels)); assert(fvm_path_draw_gradient(path, &surface, &gradient, 0) == FVM_OK);
        p = pixels + 4 * (8 * 16 + 8); assert(near(p[0], 232) && near(p[2], 23)); assert(pixels[0] == 0 && pixels[2] == 255);
        gradient.type = FVM_GRADIENT_LINEAR; gradient.matrix.x0 = 7.5;
        stop(stops, 0, 255, 0, 0, 255); stop(stops + 1, 0.5, 255, 0, 0, 255);
        stop(stops + 2, 0.5, 0, 255, 0, 255); stop(stops + 3, 1, 0, 0, 255, 255); gradient.count = 4;
        memset(pixels, 0, sizeof(pixels)); assert(fvm_path_draw_gradient(path, &surface, &gradient, 0) == FVM_OK);
        p = pixels + 4 * (8 * 16 + 7); assert(p[0] == 0 && p[1] == 255 && p[2] == 0);
        stop(stops, 0, 0, 0, 0, 255); stop(stops + 1, 1, 255, 0, 0, 255); gradient.count = 2; gradient.add[0] = 128;
        memset(pixels, 0, sizeof(pixels)); assert(fvm_path_draw_gradient(path, &surface, &gradient, 0) == FVM_OK);
        p = pixels + 4 * (8 * 16 + 7); assert(p[0] == 255 && p[1] == 0 && p[2] == 0);
        gradient.add[0] = 0; stop(stops, 0, 255, 0, 0, 128); stop(stops + 1, 1, 255, 0, 0, 128);
        for (i = 0; i < sizeof(pixels); i += 4) { pixels[i] = pixels[i + 1] = 0; pixels[i + 2] = 255; pixels[i + 3] = 128; }
        assert(fvm_path_draw_gradient(path, &surface, &gradient, 0) == FVM_OK); p = pixels + 4 * (8 * 16 + 7);
        assert(near(p[0], 170) && p[1] == 0 && near(p[2], 85) && p[3] == 192);
    }
    memset(pixels, 37, sizeof(pixels)); memcpy(original, pixels, sizeof(pixels)); invalid = gradient; invalid.count = 0;
    assert(fvm_path_draw_gradient(path, &surface, &invalid, 0) == FVM_INVALID_ARGUMENT);
    invalid.count = FVM_GRADIENT_MAX_STOPS + 1; assert(fvm_path_draw_gradient(path, &surface, &invalid, 0) == FVM_INVALID_ARGUMENT);
    invalid = gradient; invalid.stops = 0; assert(fvm_path_draw_gradient(path, &surface, &invalid, 0) == FVM_INVALID_ARGUMENT);
    invalid = gradient; invalid.matrix.xx = invalid.matrix.xy = 0; assert(fvm_path_draw_gradient(path, &surface, &invalid, 0) == FVM_INVALID_ARGUMENT);
    invalid = gradient; invalid.add[0] = HUGE_VAL; assert(fvm_path_draw_gradient(path, &surface, &invalid, 0) == FVM_INVALID_ARGUMENT);
    stops[0].ratio = 1; stops[1].ratio = 0; assert(fvm_path_draw_gradient(path, &surface, &gradient, 0) == FVM_INVALID_ARGUMENT);
    assert(memcmp(original, pixels, sizeof(pixels)) == 0);
    stops[0].ratio = 0; stops[1].ratio = 1;
    {
        FVM_PathCommand command; memset(&command, 0, sizeof(command)); command.verb = FVM_PATH_MOVE; command.x[0] = 5000;
        assert(fvm_path_add(path, &command) == FVM_OK);
        assert(fvm_path_draw_gradient(path, &surface, &gradient, 0) == FVM_INVALID_ARGUMENT);
        assert(memcmp(original, pixels, sizeof(pixels)) == 0);
    }
    fvm_path_destroy(path); return 0;
}
