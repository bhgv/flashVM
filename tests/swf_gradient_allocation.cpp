#include "flashvm/standard.h"
#include "flashvm/movieclip.h"
#include "flashvm/gc.h"
#include <cassert>
#include <cstdlib>
#include <new>
static long allowance = -1;
/* Inject a single allocation boundary through every VM new/new[] site. Both
 * allocation and deallocation use malloc/free so ASan can still observe them. */
void *operator new(std::size_t size) throw(std::bad_alloc)
{
    if (allowance == 0)
        throw std::bad_alloc();
    if (allowance > 0)
        --allowance;
    void *p = std::malloc(size ? size : 1);
    if (!p)
        throw std::bad_alloc();
    return p;
}
void operator delete(void *p) throw()
{
    std::free(p);
}
void *operator new[](std::size_t size) throw(std::bad_alloc)
{
    return ::operator new(size);
}
void operator delete[](void *p) throw()
{
    ::operator delete(p);
}
void *operator new(std::size_t size, const std::nothrow_t &) throw()
{
    try
    {
        return ::operator new(size);
    }
    catch (...)
    {
        return 0;
    }
}
void operator delete(void *p, const std::nothrow_t &) throw()
{
    ::operator delete(p);
}
void *operator new[](std::size_t size, const std::nothrow_t &) throw()
{
    try
    {
        return ::operator new[](size);
    }
    catch (...)
    {
        return 0;
    }
}
void operator delete[](void *p, const std::nothrow_t &) throw()
{
    ::operator delete[](p);
}
#define FVM_GRADIENT_FIXTURE_ONLY
#include "swf_gradient.cpp"
int main()
{
    Bytes source = gradient_shape(); bool decoded_ok = false;
    for (long failure = 0; failure < 2048 && !decoded_ok; ++failure) {
        FVM_SWFShape decoded; decoded.id = 777; allowance = failure;
        FVM_Result r = fvm_swf_read_shape(source.begin(), source.size(), 32, decoded); allowance = -1;
        assert(r == FVM_OK || r == FVM_OUT_OF_MEMORY); decoded_ok = r == FVM_OK;
        if (!decoded_ok) assert(decoded.id == 777 && decoded.paints.empty());
    }
    assert(decoded_ok); FVM_Config cfg; FVM_Runtime *rt = 0; FVM_Context *c = 0;
    assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
    Bytes tags, empty; tag(tags, 32, source); tag(tags, 26, place(2 | 4, 1, 9, 0, 0));
    tag(tags, 1, empty); tag(tags, 0, empty); Bytes swf = movie(tags, 1); bool loaded = false;
    for (long failure = 0; failure < 4096 && !loaded; ++failure) {
        FVM_Object *o = 0; allowance = failure;
        FVM_Result r = fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "root", &o); allowance = -1;
        assert(r == FVM_OK || r == FVM_OUT_OF_MEMORY); assert(r == FVM_OK || !o); loaded = r == FVM_OK;
        assert(fvm_gc_collect(c, 0) == FVM_OK);
    }
    assert(loaded); FVM_Object *root = load(c, tags, 1); tick(c, root); bool drawn = false;
    unsigned char pixels[16 * 16 * 4], original[sizeof(pixels)]; FVM_RasterSurface surface = {pixels, 16, 16, 64};
    for (long failure = 0; failure < 2048 && !drawn; ++failure) {
        std::memset(pixels, 71, sizeof(pixels)); std::memcpy(original, pixels, sizeof(pixels)); allowance = failure;
        FVM_Result r = fvm_movieclip_render(c, root, &surface); allowance = -1;
        assert(r == FVM_OK || r == FVM_OUT_OF_MEMORY || r == FVM_UNSUPPORTED);
        drawn = r == FVM_OK || r == FVM_UNSUPPORTED;
        if (!drawn) assert(std::memcmp(pixels, original, sizeof(pixels)) == 0);
        assert(fvm_gc_collect(c, 0) == FVM_OK);
    }
    assert(drawn); fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
