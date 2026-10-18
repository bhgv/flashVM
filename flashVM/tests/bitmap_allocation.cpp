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
#define FVM_BITMAP_FIXTURE_ONLY
#include "standard_bitmap.cpp"
int main() {
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0,*other=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_context_create(rt,&other)==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
    FVM_Object *source=bitmap(c,3,2,true,0x80443322);bool success=false;
    for(long failure=0;failure<2048 && !success;++failure) {
      FVM_Value out;allowance=failure;FVM_Result r=fvm_object_call(c,source,"clone",0,0,&out);allowance=-1;
      assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);success=r==FVM_OK;if(success){assert(out.object!=source);write(c,out.object,0,0,0xffffffffUL);}assert(read(c,source,0,0)==0x80443322);assert(fvm_gc_collect(c,0)==FVM_OK);
    }
    assert(success);success=false;FVM_Value shift[2];fvm_value_number(shift,1);fvm_value_number(shift+1,0);
    for(long failure=0;failure<2048 && !success;++failure){FVM_Value out;allowance=failure;FVM_Result r=fvm_object_call(c,source,"scroll",2,shift,&out);allowance=-1;assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);success=r==FVM_OK;assert(read(c,source,0,0)==0x80443322);assert(read(c,source,1,0)==0x80443322);}
    assert(success);double region[]={0,0,2,1},location[]={1,0};FVM_Object *rect=make(c,"flash.geom.Rectangle",4,region),*point=make(c,"flash.geom.Point",2,location);FVM_Value channel_args[5];channel_args[0]=ref(source);channel_args[1]=ref(rect);channel_args[2]=ref(point);fvm_value_number(channel_args+3,1);fvm_value_number(channel_args+4,1);success=false;
    for(long failure=0;failure<2048 && !success;++failure){FVM_Value out;allowance=failure;FVM_Result r=fvm_object_call(c,source,"copyChannel",5,channel_args,&out);allowance=-1;assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);success=r==FVM_OK;assert(read(c,source,0,0)==0x80443322);assert(read(c,source,1,0)==0x80443322);}
    assert(success);success=false;FVM_Value pixel_args[3]={ref(source),ref(rect),ref(point)};
    for(long failure=0;failure<2048 && !success;++failure){FVM_Value out;allowance=failure;FVM_Result r=fvm_object_call(c,source,"copyPixels",3,pixel_args,&out);allowance=-1;assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);success=r==FVM_OK;assert(read(c,source,0,0)==0x80443322);assert(read(c,source,1,0)==0x80443322);}
    assert(success);FVM_Value packet=ref(source);unsigned char *blob=0;size_t size=0;assert(fvm_value_serialize(c,&packet,&blob,&size)==FVM_OK);success=false;
    for(long failure=0;failure<20000 && !success;++failure) {
      FVM_Value restored;allowance=failure;FVM_Result r=fvm_value_deserialize(other,blob,size,&restored);allowance=-1;
      assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);success=r==FVM_OK;if(success){assert(fvm_gc_pin(other,restored.object)==FVM_OK);received(other,restored.object);}assert(fvm_gc_collect(other,0)==FVM_OK);
    }
    assert(success);fvm_serialized_free(blob);fvm_context_destroy(other);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
