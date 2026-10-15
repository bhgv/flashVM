/* Every native Try allocation boundary restores rooted state and permits a successful retry. */
#define main try_allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#define FVM_TRY_FIXTURE_ONLY
#include "jit_try.cpp"
#include "../src/avm1/block.hpp"
static void try_inactive(FVM_JitCode *code)
{assert(!code->active);for(size_t i=0;i<code->blocks.size();++i)try_inactive(code->blocks[i].code);}
int main()
{
    if(!fvm_jit_available())return 0;FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned loaded=0;loaded<2;++loaded)for(unsigned entry=0;entry<4;++entry)for(unsigned reg=0;reg<2;++reg)
    {
        FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);const unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};if(loaded)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
        Program body=try_exit(42,true),caught=try_exit(77),final;final.number(17);final.op(0x17);Program p;try_block(p,body,caught,final,reg?7:3,"e",0);p.number(99);p.finish();
        FVM_Object *fn=0;FVM_JitCode *code=0;FVM_Value out;assert(fvm_function_create_avm1(c,p.bytes.begin(),p.bytes.size(),0,0,0,&fn)==FVM_OK && fvm_gc_pin(c,fn)==FVM_OK && fvm_jit_compile(p.bytes.begin(),p.bytes.size(),&code)==FVM_OK);
        bool success=false;unsigned failures=0;
        for(long boundary=0;boundary<5000 && !success;++boundary){FVM_JitCode *candidate=code;allowance=boundary;FVM_Result status=fvm_jit_compile(p.bytes.begin(),p.bytes.size(),&candidate);allowance=-1;if(status==FVM_OK){success=true;assert(candidate!=code && fvm_jit_destroy(candidate)==FVM_OK);}else{assert(status==FVM_OUT_OF_MEMORY && candidate==code);++failures;}}
        assert(success && failures);success=false;failures=0;assert(fvm_execution_set_mode(c,entry?FVM_EXECUTION_JIT:FVM_EXECUTION_MANUAL,0)==FVM_OK);if(!entry)assert(fvm_function_set_body(c,fn,fvm_jit_body,code,0)==FVM_OK);
        for(unsigned phase=0;phase<(entry?2U:1U);++phase)
        {
            success=false;failures=0;
            for(long boundary=0;boundary<5000 && !success;++boundary)
            {
                FVM_GCRootFrame *roots=c->gc_frames;allowance=boundary;FVM_Result status=entry==3?fvm_avm1_execute_block_receiver(c,p.bytes.begin(),p.bytes.size(),c->global):entry==2?fvm_avm1_execute(c,p.bytes.begin(),p.bytes.size()):fvm_function_invoke(c,fn,0,0,0,&out);allowance=-1;
                assert(c->gc_frames==roots && c->lexical_scopes.empty() && !c->script_depth && !c->gc_native_depth && !c->avm1_execution_depth && !c->execution_mode.native_pool);try_inactive(code);
                size_t bytes=0;for(fvm_detail::map<fvm_detail::string,FVM_JitCode *>::iterator i=c->execution_mode.cache.begin();i!=c->execution_mode.cache.end();++i)if(i->second){try_inactive(i->second);bytes+=fvm_jit_code_size(i->second);}assert(bytes==c->execution_mode.cache_bytes);
                if(status==FVM_OK){success=true;if(entry<2)assert(out.type==FVM_VALUE_NUMBER && out.number==77);}else{assert(status==FVM_OUT_OF_MEMORY);++failures;}
                assert(fvm_exception_clear(c)==FVM_OK && fvm_gc_collect(c,0)==FVM_OK);
            }
            assert(success && failures);
        }
        assert(success && failures && fvm_execution_set_mode(c,FVM_EXECUTION_MANUAL,0)==FVM_OK && fvm_function_set_body(c,fn,0,0,0)==FVM_OK && fvm_jit_destroy(code)==FVM_OK);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
