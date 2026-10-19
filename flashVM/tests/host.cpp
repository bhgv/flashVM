/* Host ABI: buffer bounds, backend failures/exceptions and inverse event geometry. */
#include "flashvm/host.h"
#include <cassert>
#include <limits>
#include <new>
#include <cmath>
struct Sink { int calls, mode; const FVM_RasterSurface *source; };
static FVM_Result consume(const FVM_RasterSurface *frame, void *raw)
{
    Sink *s = (Sink *)raw; ++s->calls;
    assert(frame != s->source && frame->pixels == s->source->pixels);
    assert(frame->stride == 12 && frame->pixels[19] == 42);
    if (s->mode == 1) throw std::bad_alloc();
    if (s->mode == 2) throw 42;
    return s->mode == 3 ? FVM_UNSUPPORTED : FVM_OK;
}
int main()
{
    unsigned char bytes[20] = {0}; bytes[19] = 42;
    FVM_RasterSurface f = {bytes, 2, 2, 12}; Sink s = {0, 0, &f};
    assert(fvm_host_present(&f, 20, consume, &s) == FVM_OK && s.calls == 1);
    for (int mode = 1; mode <= 3; ++mode) {
        s.mode = mode;
        assert(fvm_host_present(&f, 20, consume, &s) ==
               (mode == 1 ? FVM_OUT_OF_MEMORY : mode == 2 ? FVM_ERROR : FVM_UNSUPPORTED));
    }
    assert(s.calls == 4);
    assert(fvm_host_present(&f, 19, consume, &s) == FVM_INVALID_ARGUMENT);
    assert(fvm_host_present(0, 20, consume, &s) == FVM_INVALID_ARGUMENT);
    assert(fvm_host_present(&f, 20, 0, &s) == FVM_INVALID_ARGUMENT);
    f.stride = 7; assert(fvm_host_present(&f, 20, consume, &s) == FVM_INVALID_ARGUMENT);
    f.stride = (size_t)-1; assert(fvm_host_present(&f, (size_t)-1, consume, &s) == FVM_INVALID_ARGUMENT);
    f.width = 0; assert(fvm_host_present(&f, 20, consume, &s) == FVM_INVALID_ARGUMENT);
    assert(s.calls == 4);
    FVM_Affine m = {2, 0, 0, 2, 100, 50}; double x = 77, y = 88;
    assert(fvm_host_pointer_to_world(&m, 120, 90, &x, &y) == FVM_OK && x == 10 && y == 20);
    m.xx = 0; m.xy = -3; m.yx = 2; m.yy = 1;
    assert(fvm_host_pointer_to_world(&m, 40, 90, &x, &y) == FVM_OK);
    assert(std::fabs(x - 10) < 1e-12 && std::fabs(y - 20) < 1e-12);
    m.xx = m.xy = m.yx = m.yy = 1;
    x = 77; y = 88;
    assert(fvm_host_pointer_to_world(&m, 0, 0, &x, &y) == FVM_INVALID_ARGUMENT && x == 77 && y == 88);
    m.xx = std::numeric_limits<double>::infinity();
    assert(fvm_host_pointer_to_world(&m, 0, 0, &x, &y) == FVM_INVALID_ARGUMENT);
    m.xx = m.yy = 1e300; m.xy = m.yx = m.x0 = m.y0 = 0;
    assert(fvm_host_pointer_to_world(&m, 1e300, -1e300, &x, &y) == FVM_OK && x == 1 && y == -1);
    assert(fvm_host_pointer_to_world(&m, 0, 0, &x, &x) == FVM_INVALID_ARGUMENT);
    assert(fvm_host_pointer_to_world(&m, std::numeric_limits<double>::quiet_NaN(), 0, &x, &y) == FVM_INVALID_ARGUMENT);
    return 0;
}
