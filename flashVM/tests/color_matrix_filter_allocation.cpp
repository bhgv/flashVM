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
#define FVM_COLOR_FILTER_FIXTURE_ONLY
#include "color_matrix_filter.cpp"
int main()
{
    long baseline=active_allocations;FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
    double original[20],replacement[20];identity_matrix(original);identity_matrix(replacement);replacement[0]=0;replacement[2]=1;replacement[10]=1;replacement[12]=0;FVM_Object *array=matrix_array(c,replacement),*effect=filter(c);FVM_Object *image=bitmap(c,3,1,true,0xffff0000UL);double box[]={0,0,2,1};FVM_Object *rect=region_object(c,"flash.geom.Rectangle",4,box),*point=hit_point(c,1,0);FVM_Value assignment=ref(array),args[4];filter_args(args,image,rect,point,effect);
    for(unsigned operation=0;operation<5;++operation){bool done=false;for(long failure=0;failure<8192 && !done;++failure){FVM_Value out;FVM_Object *created=0;allowance=failure;FVM_Result r=operation==0?fvm_construct(c,"flash.filters.ColorMatrixFilter",1,&assignment,&created):operation==1?fvm_object_set(c,effect,"matrix",&assignment):operation==2?fvm_object_get(c,effect,"matrix",&out):operation==3?fvm_object_call(c,effect,"clone",0,0,&out):fvm_object_call(c,image,"applyFilter",4,args,&out);allowance=-1;assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);done=r==FVM_OK;if(done){if(operation==0)assert(fvm_gc_pin(c,created)==FVM_OK);else if(operation==2 || operation==3)assert(fvm_gc_pin(c,out.object)==FVM_OK);}double values[20];assert(fvm_filter_matrix(c,effect,values));for(unsigned i=0;i<20;++i)assert(values[i]==(operation==0 || (operation==1 && !done)?original[i]:replacement[i]));if(operation==4 && done)assert(read(c,image,1,0)==0xff0000ffUL && read(c,image,2,0)==0xff0000ffUL);else assert(read(c,image,1,0)==0xffff0000UL && read(c,image,2,0)==0xffff0000UL);assert(fvm_gc_collect(c,0)==FVM_OK);}assert(done);}
    FVM_Value source=ref(effect);unsigned char *bytes=0;size_t size=0;assert(fvm_value_serialize(c,&source,&bytes,&size)==FVM_OK);bool done=false;for(long failure=0;failure<16384 && !done;++failure){FVM_Value out;allowance=failure;FVM_Result r=fvm_value_deserialize(c,bytes,size,&out);allowance=-1;assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);done=r==FVM_OK;if(done){double values[20];assert(fvm_filter_matrix(c,out.object,values));for(unsigned i=0;i<20;++i)assert(values[i]==replacement[i]);assert(fvm_gc_pin(c,out.object)==FVM_OK);}assert(fvm_gc_collect(c,0)==FVM_OK);}assert(done);fvm_serialized_free(bytes);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);assert(active_allocations==baseline);return 0;
}
