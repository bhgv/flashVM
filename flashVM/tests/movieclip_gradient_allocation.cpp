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
#define FVM_MOVIECLIP_GRADIENT_FIXTURE_ONLY
#include "movieclip_gradient.cpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; FVM_Context *c = 0;
    assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
    FVM_Object *root = 0; assert(fvm_movieclip_create(c, "root", 1, &root) == FVM_OK); assert(fvm_gc_pin(c, root) == FVM_OK);
    FVM_Object *clip = new_child(c, root, "gradient", 1); FVM_Value args[8], out; arguments(c, args, true);
    assert(fvm_object_call(c, clip, "beginGradientFill", 5, args, &out) == FVM_OK); geometry(c, clip, false);
    FVM_Object *data = clip->resource_result.object, *original = data->properties["gradient"].object, *active = data->properties["active"].object;
    bool changed = false;
    for (long failure = 0; failure < 2048 && !changed; ++failure) {
        allowance = failure; FVM_Result r = fvm_object_call(c, clip, "beginGradientFill", 5, args, &out); allowance = -1;
        assert(r == FVM_OK || r == FVM_OUT_OF_MEMORY); changed = r == FVM_OK;
        if (!changed) { assert(data->properties["gradient"].object == original); assert(data->properties["active"].object == active); }
        else assert(data->properties["gradient"].object != original && data->properties["active"].type == FVM_VALUE_NULL);
        assert(fvm_gc_collect(c, 0) == FVM_OK); gradient_pixels(c, clip, false);
    }
    assert(changed); fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
