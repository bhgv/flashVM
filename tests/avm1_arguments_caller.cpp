#include "flashvm/avm1.h"
#include "flashvm/function.h"
#include "flashvm/standard.h"
#include "flashvm/transfer.h"
#include "flashvm/gc.h"
#include "flashvm/property.h"
#include "flashvm/exception.h"
#include "../src/core/runtime.hpp"
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <new>
static long allowance = -1;
void *operator new(std::size_t size) throw(std::bad_alloc) { if (!allowance) throw std::bad_alloc(); if (allowance > 0) --allowance; void *p=std::malloc(size?size:1);if(!p)throw std::bad_alloc();return p; }
void operator delete(void *p) throw() { std::free(p); }
void *operator new[](std::size_t size) throw(std::bad_alloc) { return ::operator new(size); }
void operator delete[](void *p) throw() { ::operator delete(p); }
void *operator new(std::size_t size,const std::nothrow_t &) throw() { try { return ::operator new(size); } catch (...) { return 0; } }
void operator delete(void *p,const std::nothrow_t &) throw() { ::operator delete(p); }
void *operator new[](std::size_t size,const std::nothrow_t &) throw() { try { return ::operator new[](size); } catch (...) { return 0; } }
void operator delete[](void *p,const std::nothrow_t &) throw() { ::operator delete[](p); }
typedef fvm_detail::vector<unsigned char> Bytes;
static void u16(Bytes &b, unsigned n) { b.push_back((unsigned char)n); b.push_back((unsigned char)(n>>8)); }
static void text(Bytes &b,const char *s) { b.push_back(0x96);u16(b,(unsigned)std::strlen(s)+2);b.push_back(0);do { b.push_back((unsigned char)*s); } while (*s++); }
static void variable(Bytes &b,const char *s) { text(b,s);b.push_back(0x1c); }
static void call(Bytes &b,const char *s) { b.push_back(0x96);u16(b,5);b.push_back(7);for(unsigned i=0;i<4;++i)b.push_back(0);text(b,s);b.push_back(0x3d); }
static FVM_Object *make(FVM_Context *c,Bytes &b) { b.push_back(0x3e);b.push_back(0);FVM_Object *fn=0;assert(fvm_function_create_avm1(c,b.begin(),b.size(),0,0,0,&fn)==FVM_OK);return fn; }
static FVM_Value object(FVM_Object *o) { FVM_Value v;fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=o;return v; }
static FVM_Object *seen;
static FVM_Result observe(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *) { seen=c->script_callee;assert(seen);assert(fvm_gc_collect(c,0)==FVM_INVALID_ARGUMENT);fvm_value_undefined(out);return FVM_OK; }
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=4;version<=8;++version) {
        FVM_Context *c=0,*other=0;assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_context_create(rt,&other)==FVM_OK);
        unsigned char swf[]={'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
        FVM_Object *collector=0;assert(fvm_function_create_native(c,observe,0,0,&collector)==FVM_OK);FVM_Value value=object(collector);assert(fvm_set_global(c,"observe",&value)==FVM_OK);
        Bytes inner_body;call(inner_body,"observe");inner_body.push_back(0x17);variable(inner_body,"arguments");FVM_Object *inner=make(c,inner_body);value=object(inner);assert(fvm_set_global(c,"inner",&value)==FVM_OK);
        FVM_Value out,caller;assert(fvm_function_invoke(c,inner,0,0,0,&out)==FVM_OK && seen==inner);
        assert(fvm_object_get(c,out.object,"caller",&caller)==FVM_OK && caller.type==FVM_VALUE_NULL);
        unsigned flags=0;assert(fvm_object_property_flags(c,out.object,"caller",&flags)==FVM_OK && flags==1);
        Bytes outer_body;call(outer_body,"inner");FVM_Object *outer=make(c,outer_body);
        assert(fvm_function_invoke(c,outer,0,0,0,&out)==FVM_OK && seen==inner);
        FVM_Object *arguments=out.object;assert(fvm_gc_pin(c,arguments)==FVM_OK);
        assert(!c->script_depth && !c->script_callee && !c->gc_frames);
        assert(fvm_gc_collect(c,0)==FVM_OK);assert(fvm_object_get(c,arguments,"caller",&caller)==FVM_OK && caller.object==outer);
        FVM_Channel *channel=0;assert(fvm_channel_create(&channel)==FVM_OK);value=object(arguments);assert(fvm_channel_send_value(c,channel,&value,0)==FVM_OK);assert(fvm_channel_receive_value(other,channel,&out,0)==FVM_OK);
        assert(fvm_gc_pin(other,out.object)==FVM_OK);assert(fvm_gc_collect(other,0)==FVM_OK);
        assert(fvm_object_get(other,out.object,"caller",&caller)==FVM_OK && caller.type==FVM_VALUE_OBJECT && caller.object->owner==other && caller.object!=outer);
        FVM_Value callee;assert(fvm_object_get(other,out.object,"callee",&callee)==FVM_OK && callee.type==FVM_VALUE_OBJECT && callee.object!=caller.object);
        bool completed=false;unsigned failures=0;
        for(long point=0;point<2048;++point) {
            allowance=point;FVM_Result result=fvm_function_invoke(c,outer,0,0,0,&out);allowance=-1;
            assert(!c->script_depth && !c->script_callee && !c->gc_frames);
            assert(!c->gc_native_depth && c->lexical_scopes.empty());
            if(result==FVM_OK) { completed=true;break; }
            assert(result==FVM_OUT_OF_MEMORY);++failures;
        }
        assert(completed && failures);
        Bytes failure_body;text(failure_body,"failure");failure_body.push_back(0x2a);FVM_Object *failure=make(c,failure_body);value=object(failure);assert(fvm_set_global(c,"inner",&value)==FVM_OK);
        assert(fvm_function_invoke(c,outer,0,0,0,&out)==FVM_EXCEPTION);assert(!c->script_depth && !c->script_callee && !c->gc_frames);
        assert(fvm_exception_clear(c)==FVM_OK);
        assert(fvm_function_invoke(c,inner,0,0,0,&out)==FVM_OK);assert(fvm_object_get(c,out.object,"caller",&caller)==FVM_OK && caller.type==FVM_VALUE_NULL);
        fvm_channel_destroy(channel);fvm_context_destroy(other);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
