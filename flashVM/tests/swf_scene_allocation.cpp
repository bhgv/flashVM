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
#define FVM_SCENE_FIXTURE_ONLY
#include "swf_scene.cpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; FVM_Context *c = 0;
    assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
    Bytes tags, empty; definition(tags);
    tag(tags, 26, place(2 | 4 | 8 | 16 | 32, 1, 7, 20, 40, "actor", true));
    tag(tags, 1, empty); tag(tags, 0, empty); Bytes swf = movie(tags, 1);
    bool loaded = false;
    for (long failure = 0; failure < 4096 && !loaded; ++failure) {
        FVM_Object *o = 0; allowance = failure;
        FVM_Result r = fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "root", &o);
        allowance = -1; assert(r == FVM_OK || r == FVM_OUT_OF_MEMORY); assert(r == FVM_OK || !o);
        loaded = r == FVM_OK; assert(fvm_gc_collect(c, 0) == FVM_OK);
    }
    assert(loaded); bool advanced = false;
    for (long failure = 0; failure < 2048 && !advanced; ++failure) {
        FVM_Object *o = load(c, tags, 1); allowance = failure;
        FVM_Result r = fvm_movieclip_advance(c, o, 1); allowance = -1;
        assert(r == FVM_OK || r == FVM_OUT_OF_MEMORY); advanced = r == FVM_OK;
        assert(fvm_gc_collect(c, 0) == FVM_OK);
        if (advanced) { FVM_Object *actor = get(c, o, "actor").object; assert(actor); pose(c, actor, 1, 2, true); }
        assert(fvm_gc_unpin(c, o) == FVM_OK); assert(fvm_gc_collect(c, 0) == FVM_OK);
    }
    assert(advanced);
    FVM_Object *o = 0; assert(fvm_movieclip_create(c, "pose", 1, &o) == FVM_OK); assert(fvm_gc_pin(c, o) == FVM_OK);
    FVM_Affine initial, desired, actual; assert(fvm_movieclip_get_matrix(c, o, &initial) == FVM_OK);
    desired = initial; desired.xx = 2; desired.xy = 1; desired.yx = 0.5; desired.x0 = 7; bool changed = false;
    for (long failure = 0; failure < 512 && !changed; ++failure) {
        allowance = failure; FVM_Result r = fvm_movieclip_set_matrix(c, o, &desired); allowance = -1;
        assert(r == FVM_OK || r == FVM_OUT_OF_MEMORY); changed = r == FVM_OK;
        assert(fvm_movieclip_get_matrix(c, o, &actual) == FVM_OK);
        if (!changed) assert(actual.xx == initial.xx && actual.xy == initial.xy && actual.yx == initial.yx && actual.x0 == initial.x0);
        else assert(std::fabs(actual.xx - desired.xx) < 1e-12 && std::fabs(actual.xy - desired.xy) < 1e-12 && actual.x0 == desired.x0);
    }
    assert(changed); fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
