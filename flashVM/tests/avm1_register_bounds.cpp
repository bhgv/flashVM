/* Loaded register misses produce undefined and discard stores/preloads instead of failing. */
#define main function2_fixture_main
#include "avm1_function2.cpp"
#undef main
#include "../src/core/runtime.hpp"
static FVM_Object *loaded_define(FVM_Context *c,unsigned count,unsigned flags,
                                unsigned params,const char *const *names,
                                const unsigned *destinations,const Bytes &body)
{
    Bytes header,code;name(header,"overflowFunction");u16(header,params);header.push_back((unsigned char)count);u16(header,flags);
    for(unsigned i=0;i<params;++i) { header.push_back((unsigned char)destinations[i]);name(header,names[i]); }
    u16(header,(unsigned)body.size());code.push_back(0x8e);u16(code,(unsigned)header.size());
    for(size_t i=0;i<header.size();++i)code.push_back(header[i]);for(size_t i=0;i<body.size();++i)code.push_back(body[i]);code.push_back(0);
    FVM_AVM1Info info;info.struct_size=sizeof(info);
    assert(fvm_avm1_inspect(code.begin(),code.size(),&info)==FVM_INVALID_ARGUMENT);
    assert(fvm_avm1_execute(c,code.begin(),code.size())==FVM_OK);
    FVM_Value out;assert(fvm_get_global(c,"overflowFunction",&out)==FVM_OK && out.type==FVM_VALUE_OBJECT);return out.object;
}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=4;version<=9;++version) {
        FVM_Context *c=0,*other=0;assert(fvm_context_create(rt,&c)==FVM_OK && fvm_context_create(rt,&other)==FVM_OK);
        if(version<=8) { unsigned char swf[]={'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_load_memory(other,swf,sizeof(swf))==FVM_OK); }
        assert(fvm_standard_install(c)==FVM_OK && fvm_standard_install(other)==FVM_OK);
        Bytes body;integer(body,42);body.push_back(0x87);u16(body,1);body.push_back(255);body.push_back(0x17);reg(body,255);end(body);
        FVM_Object *fn=define(c,1,0,0,0,0,body);FVM_Value out;
        FVM_Result status=fvm_function_invoke(c,fn,0,0,0,&out);
        if(version==9) { assert(status==FVM_INVALID_ARGUMENT);fvm_context_destroy(other);fvm_context_destroy(c);continue; }
        assert(status==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
        Bytes throwing;text(throwing,"failure");throwing.push_back(0x2a);
        Bytes caught;reg(caught,255);caught.push_back(0x3e);
        Bytes trap;trap.push_back(0x8f);u16(trap,8);trap.push_back(5);u16(trap,(unsigned)throwing.size());u16(trap,(unsigned)caught.size());u16(trap,0);trap.push_back(255);
        for(size_t i=0;i<throwing.size();++i)trap.push_back(throwing[i]);for(size_t i=0;i<caught.size();++i)trap.push_back(caught[i]);trap.push_back(0);
        assert(fvm_function_create_avm1(c,trap.begin(),trap.size(),0,0,0,&fn)==FVM_OK);
        assert(fvm_function_invoke(c,fn,0,0,0,&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
        const char *params[]={"x"};unsigned destinations[]={255};
        for(unsigned count=0;count<=2;++count) {
            Bytes lookup;variable(lookup,"x");end(lookup);
            fn=loaded_define(c,count,1|4|16|64|128|256,1,params,destinations,lookup);
            FVM_Value global_value;fvm_value_number(&global_value,7);assert(fvm_set_global(c,"x",&global_value)==FVM_OK && fvm_set_global(other,"x",&global_value)==FVM_OK);
            FVM_Value arg;fvm_value_number(&arg,99);assert(fvm_function_invoke(c,fn,0,1,&arg,&out)==FVM_OK && out.type==FVM_VALUE_NUMBER && out.number==7);
            assert(fvm_gc_pin(c,fn)==FVM_OK && fvm_gc_collect(c,0)==FVM_OK);
            FVM_Channel *channel=0;assert(fvm_channel_create(&channel)==FVM_OK);out.type=FVM_VALUE_OBJECT;out.object=fn;
            assert(fvm_channel_send_value(c,channel,&out,0)==FVM_OK && fvm_channel_receive_value(other,channel,&out,0)==FVM_OK);
            assert(fvm_function_invoke(other,out.object,0,1,&arg,&out)==FVM_OK && out.type==FVM_VALUE_NUMBER && out.number==7);
            fvm_channel_destroy(channel);
        }
        assert(!c->gc_frames && !c->script_depth && !c->avm1_target_active);
        fvm_context_destroy(other);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
