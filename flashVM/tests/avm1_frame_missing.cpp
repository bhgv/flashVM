/* Frame Call/GotoFrame2 empty inputs use versioned labels; malformed payloads still fail first. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include "execution_value_callbacks.cpp"
#include "../src/core/runtime.hpp"
static void run(FVM_Context *c,FVM_Object *root,Bytes code)
{code.push_back(0);assert(fvm_avm1_execute_receiver(c,code.begin(),code.size(),root)==FVM_OK);}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned mode=0;mode<6;++mode)
    {
        FVM_Context *c=0;FVM_Object *root=0;unsigned version=mode+3;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_context_create(rt,&c)==FVM_OK);if(mode)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);assert(fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"root",3,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);
        const unsigned char empty[]={0};Bytes body;text(body,"seen");integer(body,3);body.push_back(0x1d);body.push_back(0);
        assert(fvm_movieclip_define_frame(c,root,1,"first",empty,sizeof(empty))==FVM_OK && fvm_movieclip_define_frame(c,root,2,"second",empty,sizeof(empty))==FVM_OK && fvm_movieclip_define_frame(c,root,3,"undefined",body.begin(),body.size())==FVM_OK);
        Bytes call;call.push_back(0x9e);u16(call,0);call.push_back(0);FVM_Value out;
        FVM_Result r=fvm_avm1_execute_receiver(c,call.begin(),call.size(),root);if(!mode)assert(r==FVM_ERROR);else{assert(r==FVM_OK);assert(fvm_object_get(c,root,"seen",&out)==FVM_OK);if(version>=7)assert(out.number==3);else assert(out.type==FVM_VALUE_UNDEFINED);}
        for(unsigned bias=0;bias<2;++bias)
        {
            Bytes code;code.push_back(0x81);u16(code,2);u16(code,0);code.push_back(0x9f);u16(code,bias?3:1);code.push_back(bias?2:0);if(bias)u16(code,65535);code.push_back(0);
            r=fvm_avm1_execute_receiver(c,code.begin(),code.size(),root);if(!mode)assert(r==FVM_ERROR);else{assert(r==FVM_OK && fvm_object_get(c,root,"_currentframe",&out)==FVM_OK && out.number==(version>=7?3:1));}
        }
        if(mode){Bytes code;text(code,"002");code.push_back(0x9f);u16(code,3);code.push_back(2);u16(code,1);run(c,root,code);assert(fvm_object_get(c,root,"_currentframe",&out)==FVM_OK && out.number==3);}
        const unsigned char bad_call[]={0x9e,1,0,0,0},bad_goto[]={0x9f,1,0,4,0};
        assert(fvm_avm1_execute_receiver(c,bad_call,sizeof(bad_call),root)==FVM_INVALID_ARGUMENT && fvm_avm1_execute_receiver(c,bad_goto,sizeof(bad_goto),root)==FVM_INVALID_ARGUMENT);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
