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
#define FVM_CONTENT_INFO_FIXTURE_ONLY
#include "movieclip_content_info.cpp"
/* Exhaust query allocation boundaries without publishing a partial caller descriptor. */
int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Object *clip = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK);
    assert(fvm_movieclip_load_swf(c, movie, sizeof(movie), "loaded", &clip) == FVM_OK &&
           fvm_gc_pin(c, clip) == FVM_OK);
    bool success = false;
    unsigned failures = 0;
    for (long budget = 0; budget < 4096 && !success; ++budget)
    {
        FVM_MovieClipContentInfo info = sentinel(), before = info;
        allowance = budget;
        FVM_Result status = fvm_movieclip_get_content_info(c, clip, &info);
        allowance = -1;
        if (status == FVM_OK)
        {
            success = true;
            assert(info.encoded_bytes == sizeof(movie));
        }
        else
        {
            assert(status == FVM_OUT_OF_MEMORY);
            ++failures;
            assert(!std::memcmp(&info, &before, sizeof(info)));
        }
        assert(fvm_gc_collect(c, 0) == FVM_OK && content(c, clip).swf_version == 6);
    }
    assert(success && failures);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
