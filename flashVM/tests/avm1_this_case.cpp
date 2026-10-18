/* Versioned implicit this lookup in timeline and function scopes. */
#define main named_scope_fixture_main
#include "avm1_function2_named_scope.cpp"
#undef main
#include "flashvm/movieclip.h"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    const char *keys[]={"this","THIS","ThIs","TH\xc4\xb0S","thisSuffix"};
    for(unsigned version=5;version<=9;++version) {
        FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);
        if(version<=8) {
            unsigned char swf[]={'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};swf[3]=(unsigned char)version;
            assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
        }
        assert(fvm_standard_install(c)==FVM_OK);
        FVM_Object *self=0;assert(fvm_movieclip_create(c,"root",1,&self)==FVM_OK);
        assert(fvm_gc_pin(c,self)==FVM_OK);
        for(unsigned i=0;i<5;++i) {
            Bytes body;variable(body,keys[i]);body.push_back(0x3e);body.push_back(0);
            FVM_Object *fn=0;assert(fvm_function_create_avm1(c,body.begin(),body.size(),0,0,0,&fn)==FVM_OK);
            FVM_Value out;assert(fvm_function_invoke(c,fn,self,0,0,&out)==FVM_OK);
            bool expected=i==0 || (version==6 && i<4);
            assert(expected ? out.type==FVM_VALUE_OBJECT && out.object==self : out.type==FVM_VALUE_UNDEFINED);
            Bytes timeline;text(timeline,"result");variable(timeline,keys[i]);timeline.push_back(0x1d);timeline.push_back(0);
            assert(fvm_avm1_execute_receiver(c,timeline.begin(),timeline.size(),self)==FVM_OK);
            assert(fvm_object_get(c,self,"result",&out)==FVM_OK);
            expected=i==0 || (version<=6 && i<4);
            assert(expected ? out.type==FVM_VALUE_OBJECT && out.object==self : out.type==FVM_VALUE_UNDEFINED);
        }
        FVM_Value key,out;assert(fvm_value_string_bytes(&key,"THIS\0suffix",11)==FVM_OK);
        assert(fvm_set_global(c,"countedKey",&key)==FVM_OK);
        Bytes timeline;text(timeline,"result");variable(timeline,"countedKey");timeline.push_back(0x1c);timeline.push_back(0x1d);timeline.push_back(0);
        assert(fvm_avm1_execute_receiver(c,timeline.begin(),timeline.size(),self)==FVM_OK);
        assert(fvm_object_get(c,self,"result",&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
