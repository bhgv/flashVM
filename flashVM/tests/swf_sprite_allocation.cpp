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
int main()
{
    const unsigned char swf[] = {
        'F','W','S',8,39,0,0,0,8,0,0,12,1,0,
        0xc8,9,1,0,1,0,0x40,0,0,0,
        9,14,1,0,1,0,'H','e','r','o',0,0x40,0,0,0
    };
    FVM_Config cfg; FVM_Runtime *rt = 0; FVM_Context *c = 0;
    assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
    bool loaded = false;
    for (long failure = 0; failure < 2048 && !loaded; ++failure)
    {
        FVM_Object *clip = 0;
        allowance = failure;
        FVM_Result result = fvm_movieclip_load_swf(c, swf, sizeof(swf), "resource", &clip);
        allowance = -1;
        assert(result == FVM_OK || result == FVM_OUT_OF_MEMORY);
        assert(result == FVM_OK || !clip);
        loaded = result == FVM_OK;
        assert(fvm_gc_collect(c, 0) == FVM_OK);
    }
    assert(loaded);
    FVM_Object *root = 0;
    assert(fvm_movieclip_load_swf(c, swf, sizeof(swf), "root", &root) == FVM_OK);
    assert(fvm_gc_pin(c, root) == FVM_OK);
    FVM_Value args[3], out;
    fvm_value_string(args, "Hero"); fvm_value_string(args + 1, "child"); fvm_value_number(args + 2, 1);
    bool attached = false;
    for (long failure = 0; failure < 2048 && !attached; ++failure)
    {
        allowance = failure;
        FVM_Result result = fvm_object_call(c, root, "attachMovie", 3, args, &out);
        allowance = -1;
        assert(result == FVM_OK || result == FVM_OUT_OF_MEMORY);
        attached = result == FVM_OK;
        assert(fvm_gc_collect(c, 0) == FVM_OK);
        FVM_Value child; assert(fvm_object_get(c, root, "child", &child) == FVM_OK);
        if (!attached) assert(child.type == FVM_VALUE_UNDEFINED);
        else assert(child.type == FVM_VALUE_OBJECT);
    }
    assert(attached);
    FVM_Value child; assert(fvm_object_get(c, root, "child", &child) == FVM_OK);
    FVM_Object *source = child.object;
    fvm_value_number(args, 0xff0000);
    assert(fvm_object_call(c, source, "beginFill", 1, args, &out) == FVM_OK);
    fvm_value_number(args, 0); fvm_value_number(args + 1, 0);
    assert(fvm_object_call(c, source, "moveTo", 2, args, &out) == FVM_OK);
    fvm_value_number(args, 10); fvm_value_number(args + 1, 10);
    assert(fvm_object_call(c, source, "lineTo", 2, args, &out) == FVM_OK);
    fvm_value_string(args, "duplicate"); fvm_value_number(args + 1, 2);
    bool duplicated = false;
    for (long failure = 0; failure < 2048 && !duplicated; ++failure)
    {
        allowance = failure;
        FVM_Result result = fvm_object_call(c, source, "duplicateMovieClip", 2, args, &out);
        allowance = -1;
        assert(result == FVM_OK || result == FVM_OUT_OF_MEMORY);
        duplicated = result == FVM_OK;
        assert(fvm_gc_collect(c, 0) == FVM_OK);
        FVM_Value copy; assert(fvm_object_get(c, root, "duplicate", &copy) == FVM_OK);
        if (!duplicated) assert(copy.type == FVM_VALUE_UNDEFINED);
        else assert(copy.type == FVM_VALUE_OBJECT);
    }
    assert(duplicated);
    assert(fvm_movieclip_advance(c, root, 1) == FVM_OK);
    fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
