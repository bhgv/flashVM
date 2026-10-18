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
#define FVM_JPEG_FIXTURE_ONLY
#include "image_codecs_swf_jpeg.cpp"
int main()
{
    Bytes payload=jpeg_resource(jpeg_fixture(),true);bool decoded_ok=false;
    for(long failure=0;failure<2048 && !decoded_ok;++failure){FVM_SWFBitmap decoded;decoded.id=777;allowance=failure;
        FVM_Result r=fvm_swf_read_jpeg(payload.begin(),payload.size(),35,8,1024,decoded);allowance=-1;
        assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);decoded_ok=r==FVM_OK;if(!decoded_ok)assert(decoded.id==777 && decoded.pixels.empty());}
    assert(decoded_ok);FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);Bytes swf=jpeg_movie(true);bool loaded=false;
    for(long failure=0;failure<4096 && !loaded;++failure){FVM_Object *root=0;allowance=failure;
        FVM_Result r=fvm_movieclip_load_swf(c,swf.begin(),swf.size(),"root",&root);allowance=-1;
        assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);assert(r==FVM_OK || !root);loaded=r==FVM_OK;assert(fvm_gc_collect(c,0)==FVM_OK);}
    assert(loaded);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
