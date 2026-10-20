/* Exhaust every replacement allocation boundary; failed publication retains the target graph. */
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
#define FVM_UNLOAD_FIXTURE_ONLY
#include "movieclip_unload.cpp"
int main()
{
    FVM_Config config; FVM_Runtime *runtime = 0; FVM_Context *c = 0; FVM_Object *root = 0;
    assert(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config,&runtime) == FVM_OK);
    assert(fvm_context_create(runtime,&c) == FVM_OK && fvm_standard_install(c) == FVM_OK);
    assert(fvm_load_memory(c,movie,sizeof(movie)) == FVM_OK);
    assert(fvm_movieclip_load_swf(c,movie,sizeof(movie),"_root",&root) == FVM_OK);
    assert(fvm_gc_pin(c,root) == FVM_OK);
    FVM_Object *slot = child(c,root,"slot"), *old_child = child(c,slot,"old");
    assert(fvm_movieclip_set_source(c,root,"/exe/root.swf",13) == FVM_OK);
    bool queried = false;
    for (long boundary = 0; boundary < 4096 && !queried; ++boundary) {
        int attached = 17;
        allowance = boundary;
        FVM_Result result = fvm_movieclip_attached_to(c,old_child,root,&attached);
        allowance = -1;
        assert(result == FVM_OK || result == FVM_OUT_OF_MEMORY);
        queried = result == FVM_OK;
        assert(queried ? attached == 1 : attached == 17);
    }
    assert(queried);
    FVM_Object *before = slot->resource_result.object;
    bool unloaded = false;
    for (long boundary = 0; boundary < 4096 && !unloaded; ++boundary) {
        allowance = boundary; FVM_Result result = fvm_movieclip_unload(c,slot); allowance = -1;
        assert(result == FVM_OK || result == FVM_OUT_OF_MEMORY); unloaded = result == FVM_OK;
        FVM_Value value;
        assert(fvm_object_get(c,root,"slot",&value) == FVM_OK && value.object == slot);
        if (!unloaded) {
            assert(slot->resource_result.object == before);
            assert(fvm_object_get(c,slot,"old",&value) == FVM_OK && value.object == old_child);
            assert(fvm_exec_clip_detached(c,old_child,&value) == FVM_OK && !value.boolean);
        }
        assert(fvm_gc_collect(c,0) == FVM_OK);
    }
    assert(unloaded);
    assert(fvm_movieclip_advance(c,root,1) == FVM_OK);
    assert(fvm_gc_unpin(c,root) == FVM_OK); fvm_context_destroy(c); fvm_runtime_destroy(runtime);
    return 0;
}
