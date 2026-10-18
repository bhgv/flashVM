#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/property.h"
#include "flashvm/transfer.h"
#include "flashvm/execution.h"
#include "../src/core/runtime.hpp"
#include <cassert>
#include <cstring>
static FVM_Value reference(FVM_Object *o) { FVM_Value v; fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=o;return v; }
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=4;version<=9;++version)
    {
        FVM_Context *c=0,*other=0;assert(fvm_context_create(rt,&c)==FVM_OK && fvm_context_create(rt,&other)==FVM_OK);
        if(version<=8) { unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_load_memory(other,swf,sizeof(swf))==FVM_OK); }
        assert(fvm_standard_install(c)==FVM_OK && fvm_gc_string_mode(c,FVM_GC_STRINGS_TRACED)==FVM_OK);
        FVM_Object *o=0,*parent=0;assert(fvm_construct(c,"Object",0,0,&o)==FVM_OK && fvm_gc_pin(c,o)==FVM_OK);
        assert(fvm_construct(c,"Object",0,0,&parent)==FVM_OK && fvm_gc_pin(c,parent)==FVM_OK);
        FVM_Value value,key,out;fvm_value_string(&key,"__proto__");fvm_value_number(&value,17);
        assert(fvm_object_set(c,parent,"inherited",&value)==FVM_OK);value=reference(parent);
        assert(fvm_object_set(c,o,"__proto__",&value)==FVM_OK && o->prototype==parent);
        assert(fvm_object_get(c,o,"inherited",&out)==FVM_OK && out.number==17);
        const char raw_bytes[]={'a',0,'b'};
        for(unsigned kind=0;kind<5;++kind)
        {
            if(kind==0) fvm_value_number(&value,19);
            else if(kind==1) fvm_value_boolean(&value,1);
            else if(kind==2) fvm_value_string_bytes(&value,raw_bytes,3);
            else { fvm_value_undefined(&value);if(kind==3)value.type=FVM_VALUE_NULL; }
            FVM_Value owner=reference(o);assert(fvm_exec_member(c,1,&owner,&key,&value,&out)==FVM_OK);
            if(version==9) { assert(o->prototype==(kind<3?parent:0));continue; }
            assert(!o->prototype && fvm_object_get(c,o,"__proto__",&out)==FVM_OK && out.type==value.type);
            if(kind==2) assert(out.string_size==3 && !std::memcmp(out.string,raw_bytes,3));
            if(kind==2)
            {
                FVM_Channel *ch=0; FVM_Value sent=reference(o),received,raw;
                assert(fvm_channel_create(&ch)==FVM_OK && fvm_channel_send_value(c,ch,&sent,0)==FVM_OK && fvm_channel_receive_value(other,ch,&received,0)==FVM_OK);
                assert(fvm_object_get(other,received.object,"__proto__",&raw)==FVM_OK && raw.type==FVM_VALUE_STRING && raw.string_size==3 && !std::memcmp(raw.string,raw_bytes,3));fvm_channel_destroy(ch);
            }
            if(kind==0) assert(out.number==19);
            assert(fvm_object_get(c,o,"inherited",&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
            int own=0;assert(fvm_object_has_own(c,o,"__proto__",&own)==FVM_OK && own);
            unsigned flags=0;assert(fvm_object_property_flags(c,o,"__proto__",&flags)==FVM_OK && flags==3);
            assert(fvm_gc_collect(c,0)==FVM_OK && fvm_object_get(c,o,"__proto__",&out)==FVM_OK && out.type==value.type);
        }
        if(version<=8)
        {
            assert(fvm_object_set_prototype(c,o,parent)==FVM_OK && fvm_object_get(c,o,"__proto__",&out)==FVM_OK && out.object==parent);
            value=reference(o);assert(fvm_object_set(c,o,"__proto__",&value)==FVM_INVALID_ARGUMENT && o->prototype==parent);
            FVM_Channel *channel=0;assert(fvm_channel_create(&channel)==FVM_OK);assert(fvm_channel_send_value(c,channel,&value,0)==FVM_OK && fvm_channel_receive_value(other,channel,&out,0)==FVM_OK);
            assert(fvm_object_get(other,out.object,"inherited",&value)==FVM_OK && value.number==17);fvm_channel_destroy(channel);
            assert(fvm_object_set_property_flags(c,o,"__proto__",0,3)==FVM_OK);
            fvm_value_number(&value,31);assert(fvm_object_set(c,o,"__proto__",&value)==FVM_OK);
            unsigned flags=3;assert(fvm_object_property_flags(c,o,"__proto__",&flags)==FVM_OK && flags==0);
            int deleted=0;
            assert(fvm_object_delete(c,o,"__proto__",&deleted)==FVM_OK && deleted && !o->prototype);
            assert(fvm_object_get(c,o,"__proto__",&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
        }
        fvm_context_destroy(other);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
