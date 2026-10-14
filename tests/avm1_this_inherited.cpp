/* Function2 named this inherits caller identity independently of receiver preload. */
#define main named_scope_fixture_main
#include "avm1_function2_named_scope.cpp"
#undef main
#include "flashvm/exception.h"
static void integer(Bytes &b,unsigned n) { b.push_back(0x96);u16(b,5);b.push_back(7);for(unsigned i=0;i<4;++i)b.push_back((unsigned char)(n>>(8*i))); }
struct Probe { FVM_Object *expected,*receiver; unsigned calls; };
static FVM_Result reenter(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *raw)
{
    Probe *probe=(Probe *)raw;assert(c->script_this==probe->expected);
    assert(fvm_gc_collect(c,0)==FVM_INVALID_ARGUMENT);
    Bytes code;text(code,"reenteredThis");variable(code,"this");code.push_back(0x1d);code.push_back(0);
    assert(fvm_avm1_execute_receiver(c,code.begin(),code.size(),probe->receiver)==FVM_OK);
    FVM_Value observed;assert(fvm_get_global(c,"reenteredThis",&observed)==FVM_OK && observed.type==FVM_VALUE_OBJECT && observed.object==probe->receiver);
    assert(c->script_this==probe->expected);++probe->calls;fvm_value_undefined(out);return FVM_OK;
}
static FVM_Object *inner(FVM_Context *c,unsigned flags,bool reg,bool throwing)
{
    Bytes body;integer(body,0);text(body,"probe");body.push_back(0x3d);body.push_back(0x17);
    if(reg) { body.push_back(0x96);u16(body,2);body.push_back(4);body.push_back(1); }
    else variable(body,"this");
    body.push_back(throwing?0x2a:0x3e);body.push_back(0);
    Bytes header;name(header,"");u16(header,0);header.push_back(flags&1?1:0);u16(header,flags);u16(header,(unsigned)body.size());
    Bytes outer;outer.push_back(0x8e);u16(outer,(unsigned)header.size());
    for(size_t i=0;i<header.size();++i)outer.push_back(header[i]);
    for(size_t i=0;i<body.size();++i)outer.push_back(body[i]);
    outer.push_back(0x3e);outer.push_back(0);
    FVM_Object *factory=0;assert(fvm_function_create_avm1(c,outer.begin(),outer.size(),0,0,0,&factory)==FVM_OK);
    FVM_Value out;assert(fvm_function_invoke(c,factory,0,0,0,&out)==FVM_OK && out.type==FVM_VALUE_OBJECT);return out.object;
}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=5;version<=8;++version) {
        FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);
        unsigned char swf[]={'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
        FVM_Object *caller=0,*receiver=0;assert(fvm_construct(c,"Object",0,0,&caller)==FVM_OK && fvm_construct(c,"Object",0,0,&receiver)==FVM_OK);
        assert(fvm_gc_pin(c,caller)==FVM_OK);
        FVM_Value value;fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=receiver;assert(fvm_set_global(c,"other",&value)==FVM_OK);
        Probe probe={0,receiver,0};FVM_Object *native=0;assert(fvm_function_create_native(c,reenter,&probe,receiver,&native)==FVM_OK);value.object=native;assert(fvm_set_global(c,"probe",&value)==FVM_OK);
        Bytes body;integer(body,0);variable(body,"other");text(body,"method");body.push_back(0x52);body.push_back(0x3e);body.push_back(0);
        FVM_Object *outer=0;assert(fvm_function_create_avm1(c,body.begin(),body.size(),0,0,0,&outer)==FVM_OK);assert(fvm_gc_pin(c,outer)==FVM_OK);
        for(unsigned flags=0;flags<4;++flags)for(unsigned mode=0;mode<3;++mode) {
            bool reg=mode==1,throwing=mode==2;if(reg && !(flags&1))continue;
            FVM_Object *fn=inner(c,flags,reg,throwing);value.object=fn;assert(fvm_object_set(c,receiver,"method",&value)==FVM_OK);assert(fvm_gc_collect(c,0)==FVM_OK);
            probe.expected=flags?caller:receiver;unsigned before=probe.calls;
            FVM_Value out;FVM_Result result=fvm_function_invoke(c,outer,caller,0,0,&out);
            assert(probe.calls==before+1);
            if(throwing) { assert(result==FVM_EXCEPTION);assert(fvm_exception_clear(c)==FVM_OK); }
            else {
                assert(result==FVM_OK);
                if(reg && flags==3)assert(out.type==FVM_VALUE_UNDEFINED);
                else assert(out.type==FVM_VALUE_OBJECT && out.object==(reg || !flags ? receiver : caller));
            }
            assert(!c->script_this && !c->script_depth && !c->script_callee && !c->gc_frames);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
