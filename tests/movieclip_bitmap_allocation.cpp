#include "flashvm/standard.h"
#include "flashvm/movieclip.h"
#include "flashvm/gc.h"
#include <cassert>
#include <cstdlib>
#include <new>
static long allowance = -1;
static long active_allocations = 0;
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
    ++active_allocations;
    return p;
}
void operator delete(void *p) throw()
{
    if (p) --active_allocations;
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
#define FVM_BITMAP_FILL_FIXTURE_ONLY
#include "movieclip_bitmap_fill.cpp"
int main(){long baseline=active_allocations;FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);FVM_Object *root=0;assert(fvm_movieclip_create(c,"root",1,&root)==FVM_OK);assert(fvm_gc_pin(c,root)==FVM_OK);FVM_Object *clip=child(c,root,"texture",1),*image=bitmap(c,2,1,true,0xff0000ffUL);write(c,image,1,0,0xff00ff00UL);double m[]={2,0,0,2,0,0};FVM_Object *matrix=make(c,"flash.geom.Matrix",6,m);begin(c,clip,image,matrix);geometry(c,clip,false);FVM_Object *data=clip->resource_result.object,*descriptor=data->properties["bitmap"].object,*active=data->properties["active"].object;FVM_Value args[4]={ref(image),ref(matrix)};fvm_value_boolean(args+2,1);fvm_value_boolean(args+3,0);bool success=false;
for(long failure=0;failure<2048 && !success;++failure){FVM_Value out;allowance=failure;FVM_Result r=fvm_object_call(c,clip,"beginBitmapFill",4,args,&out);allowance=-1;assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);success=r==FVM_OK;if(!success){assert(data->properties["bitmap"].object==descriptor);assert(data->properties["active"].object==active);}assert(fvm_gc_collect(c,0)==FVM_OK);verify(c,clip,255);}assert(success);
unsigned char pixels[256],before[256];FVM_RasterSurface surface={pixels,8,8,32};std::memset(pixels,37,256);std::memcpy(before,pixels,256);success=false;
for(long failure=0;failure<4096 && !success;++failure){allowance=failure;FVM_Result r=fvm_movieclip_render(c,clip,&surface);allowance=-1;assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY || r==FVM_UNSUPPORTED);success=r==FVM_OK || r==FVM_UNSUPPORTED;if(r==FVM_OUT_OF_MEMORY)assert(std::memcmp(before,pixels,256)==0);}assert(success);
fvm_context_destroy(c);fvm_runtime_destroy(rt);assert(active_allocations==baseline);return 0;}
