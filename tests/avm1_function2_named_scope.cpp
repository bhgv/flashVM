#include "flashvm/avm1.h"
#include "flashvm/function.h"
#include "flashvm/standard.h"
#include "flashvm/table.h"
#include "flashvm/transfer.h"
#include "flashvm/gc.h"
#include "../src/core/runtime.hpp"
#include <cassert>
#include <cstring>
typedef fvm_detail::vector<unsigned char> Bytes;
static void u16(Bytes &b,unsigned n) { b.push_back((unsigned char)n);b.push_back((unsigned char)(n>>8)); }
static void name(Bytes &b,const char *s) { do { b.push_back((unsigned char)*s); } while(*s++); }
static void text(Bytes &b,const char *s) { b.push_back(0x96);u16(b,(unsigned)std::strlen(s)+2);b.push_back(0);name(b,s); }
static void variable(Bytes &b,const char *s) { text(b,s);b.push_back(0x1c); }
static FVM_Object *nested(FVM_Context *c,FVM_Object *self,unsigned flags,bool preloaded,bool parameter)
{
    bool arguments=(flags&12)!=0;const char *key=arguments?"arguments":"super";
    Bytes body;
    if(preloaded) { body.push_back(0x96);u16(body,2);body.push_back(4);body.push_back(1); }
    else variable(body,key);
    body.push_back(0x3e);body.push_back(0);
    Bytes header;name(header,"");u16(header,parameter?1:0);header.push_back((flags&(4|16))?1:0);u16(header,flags);
    if(parameter) { header.push_back(0);name(header,key); }
    u16(header,(unsigned)body.size());
    Bytes outer;text(outer,"super");text(outer,"outer-super");outer.push_back(0x1d);
    outer.push_back(0x8e);u16(outer,(unsigned)header.size());
    for(size_t i=0;i<header.size();++i)outer.push_back(header[i]);
    for(size_t i=0;i<body.size();++i)outer.push_back(body[i]);
    outer.push_back(0x3e);outer.push_back(0);
    FVM_Object *factory=0;assert(fvm_function_create_avm1(c,outer.begin(),outer.size(),0,0,0,&factory)==FVM_OK);
    FVM_Value arg,out;fvm_value_string(&arg,"outer-argument");assert(fvm_function_invoke(c,factory,self,1,&arg,&out)==FVM_OK && out.type==FVM_VALUE_OBJECT);return out.object;
}
static void expect(FVM_Context *c,FVM_Value out,bool arguments,bool preloaded,bool parameter)
{
    if(arguments && !parameter) {
        assert(out.type==FVM_VALUE_OBJECT && out.object->resource_kind==28);
        assert(fvm_table_get_index(c,out.object,0,&out)==FVM_OK);
        assert(out.type==FVM_VALUE_STRING && !std::strcmp(out.string,preloaded?"inner-argument":"outer-argument"));
    } else if(preloaded) assert(out.type==FVM_VALUE_UNDEFINED);
    else assert(out.type==FVM_VALUE_STRING && !std::strcmp(out.string,parameter?"inner-argument":"outer-super"));
}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    const unsigned flags[]={4,8,12,16,32,48};
    for(unsigned version=4;version<=8;++version) {
        FVM_Context *c=0,*other=0;assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_context_create(rt,&other)==FVM_OK);
        unsigned char swf[]={'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);assert(fvm_load_memory(other,swf,sizeof(swf))==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);assert(fvm_standard_install(other)==FVM_OK);
        FVM_Object *self=0;assert(fvm_construct(c,"Object",0,0,&self)==FVM_OK);assert(fvm_gc_pin(c,self)==FVM_OK);
        FVM_Channel *channel=0;assert(fvm_channel_create(&channel)==FVM_OK);
        for(unsigned i=0;i<6;++i)for(unsigned mode=0;mode<3;++mode) {
            bool preloaded=mode==1,parameter=mode==2;
            if(preloaded && !(flags[i]&4) && flags[i]!=48)continue;
            FVM_Object *fn=nested(c,self,flags[i],preloaded,parameter);assert(fvm_gc_pin(c,fn)==FVM_OK);assert(fvm_gc_collect(c,0)==FVM_OK);
            FVM_Value arg,out;fvm_value_string(&arg,"inner-argument");assert(fvm_function_invoke(c,fn,self,1,&arg,&out)==FVM_OK);expect(c,out,(flags[i]&12)!=0,preloaded,parameter);
            FVM_Value value;fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=fn;
            assert(fvm_channel_send_value(c,channel,&value,0)==FVM_OK);assert(fvm_channel_receive_value(other,channel,&out,0)==FVM_OK);FVM_Object *copy=out.object;
            assert(fvm_gc_pin(other,copy)==FVM_OK);assert(fvm_gc_collect(other,0)==FVM_OK);assert(fvm_function_invoke(other,copy,0,1,&arg,&out)==FVM_OK);expect(other,out,(flags[i]&12)!=0,preloaded,parameter);
            assert(fvm_gc_unpin(other,copy)==FVM_OK);assert(fvm_gc_unpin(c,fn)==FVM_OK);
        }
        assert(!c->script_callee && !c->script_depth && !c->gc_frames && !other->script_callee);fvm_channel_destroy(channel);fvm_context_destroy(other);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
