/* Native named lookup preserves selected timelines, qualified receivers and literal constructor names. */
#define main reference_scope_fixture_main
#include "jit_references.cpp"
#undef main
#include "flashvm/movieclip.h"
#include <cstdio>
static void lookup(FVM_Context *c,const char *key,FVM_Object *receiver,FVM_Object *expected)
{
    Program p;p.number(0);p.text(key);p.op(0x3d);p.finish();FVM_Object *fn=0;
    assert(fvm_function_create_avm1(c,p.bytes.begin(),p.bytes.size(),0,0,0,&fn)==FVM_OK);
    assert(fvm_gc_pin(c,fn)==FVM_OK);FVM_JitCode *code=0;FVM_Value out;
    assert(fvm_jit_compile(p.bytes.begin(),p.bytes.size(),&code)==FVM_OK);
    for(unsigned mode=0;mode<3;++mode)
    {
        assert(fvm_execution_set_mode(c,mode==2?FVM_EXECUTION_JIT:FVM_EXECUTION_MANUAL,0)==FVM_OK);
        assert(fvm_function_set_body(c,fn,mode==1?fvm_jit_body:0,mode==1?code:0,0)==FVM_OK);
        unsigned long before=c->execution_mode.stats.native_entries;
        assert(fvm_function_invoke(c,fn,receiver,0,0,&out)==FVM_OK);
        if(out.type!=FVM_VALUE_OBJECT || out.object!=expected)std::fprintf(stderr,"scope key=%s mode=%u got=%p expected=%p receiver=%p base=%p target=%p global=%p\n",key,mode,(void*)out.object,(void*)expected,(void*)receiver,(void*)c->avm1_base_clip,(void*)c->avm1_target_clip,(void*)c->global);
        assert(out.type==FVM_VALUE_OBJECT && out.object==expected);
        assert(c->execution_mode.stats.native_entries==before+(mode==2?2:mode));
    }
    assert(fvm_execution_set_mode(c,FVM_EXECUTION_MANUAL,0)==FVM_OK);
    assert(fvm_function_set_body(c,fn,0,0,0)==FVM_OK && fvm_jit_destroy(code)==FVM_OK);
}
int main()
{
    if(!fvm_jit_available())return 0;
    FVM_Config cfg;FVM_Runtime *rt=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=5;version<=8;++version)
    {
        FVM_Context *c=0;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
        swf[3]=(unsigned char)version;assert(fvm_context_create(rt,&c)==FVM_OK);
        assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
        FVM_Object *base=0,*selected=0,*owner=0,*method=0;FVM_Value value;
        assert(fvm_construct(c,"MovieClip",0,0,&base)==FVM_OK && fvm_gc_pin(c,base)==FVM_OK);
        assert(fvm_construct(c,"MovieClip",0,0,&selected)==FVM_OK && fvm_gc_pin(c,selected)==FVM_OK);
        assert(fvm_object_create(c,&owner)==FVM_OK && fvm_gc_pin(c,owner)==FVM_OK);
        Program body;body.variable("this");body.finish();
        assert(fvm_function_create_avm1(c,body.bytes.begin(),body.bytes.size(),0,0,0,&method)==FVM_OK);
        fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=method;
        assert(fvm_set_global(c,"readThis",&value)==FVM_OK);
        assert(fvm_object_set(c,owner,"method",&value)==FVM_OK);
        value.object=owner;assert(fvm_set_global(c,"owner",&value)==FVM_OK);
        c->avm1_base_clip=base;
        lookup(c,"readThis",base,base);
        c->avm1_target_active=true;c->avm1_target_clip=selected;
        /* A new prepared activation resets an inherited caller SetTarget. */
        lookup(c,"readThis",base,base);
        /* Fresh root entries reset inherited SetTarget and restore it after both native entries. */
        Program root;root.text("observed");root.number(0);root.text("readThis");root.op(0x3d);
        root.op(0x1d);root.op(0);
        for(unsigned native=0;native<2;++native)
        {
            assert(fvm_execution_set_mode(c,native?FVM_EXECUTION_JIT:FVM_EXECUTION_MANUAL,0)==FVM_OK);
            unsigned long before=c->execution_mode.stats.native_entries;
            assert(fvm_avm1_execute_receiver(c,root.bytes.begin(),root.bytes.size(),base)==FVM_OK);
            assert(fvm_object_get(c,base,"observed",&value)==FVM_OK &&
                   value.type==FVM_VALUE_OBJECT && value.object==base);
            assert(c->execution_mode.stats.native_entries==before+(native?2:0));
            assert(c->avm1_target_clip==selected && c->avm1_base_clip==base);
        }
        assert(fvm_execution_set_mode(c,FVM_EXECUTION_MANUAL,0)==FVM_OK);
        lookup(c,"owner:method",base,owner);lookup(c,"owner.method",base,owner);
        c->avm1_base_clip=0;c->avm1_target_clip=0;c->avm1_target_active=false;
        assert(!c->gc_frames && c->lexical_scopes.empty());fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
