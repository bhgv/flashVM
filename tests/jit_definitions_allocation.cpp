/* Compile and execute definitions under every allocation boundary without leaking active roots. */
#define main definition_allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#define FVM_DEFINITIONS_FIXTURE_ONLY
#include "jit_definitions.cpp"
int main()
{
    if(!fvm_jit_available())return 0;
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned loaded=0;loaded<2;++loaded)for(unsigned second=0;second<2;++second)for(unsigned named=0;named<2;++named)for(unsigned automatic=0;automatic<2;++automatic)
    {
        FVM_Context *c=0;const unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
        assert(fvm_context_create(rt,&c)==FVM_OK);if(loaded)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
        assert(fvm_standard_install(c)==FVM_OK);
        Program body;if(second)definition_register(body,2);else body.variable("x");body.finish();Program p;definition_pool(p,"captured");definition_record(p,named?"made":"",second,named,body);if(named)p.variable("made");p.finish();
        FVM_Object *fn=0;FVM_JitCode *code=0;FVM_Value out;
        assert(fvm_function_create_avm1(c,p.bytes.begin(),p.bytes.size(),0,0,0,&fn)==FVM_OK && fvm_gc_pin(c,fn)==FVM_OK && fvm_jit_compile(p.bytes.begin(),p.bytes.size(),&code)==FVM_OK);
        bool success=false;unsigned failures=0;
        for(long boundary=0;boundary<3000 && !success;++boundary)
        {
            FVM_JitCode *candidate=code;allowance=boundary;FVM_Result status=fvm_jit_compile(p.bytes.begin(),p.bytes.size(),&candidate);allowance=-1;
            if(status==FVM_OK){success=true;assert(candidate!=code && fvm_jit_destroy(candidate)==FVM_OK);}else{assert(status==FVM_OUT_OF_MEMORY && candidate==code);++failures;}
        }
        assert(success && failures);success=false;failures=0;
        assert(fvm_execution_set_mode(c,automatic?FVM_EXECUTION_JIT:FVM_EXECUTION_MANUAL,0)==FVM_OK);
        if(!automatic)assert(fvm_function_set_body(c,fn,fvm_jit_body,code,0)==FVM_OK);
        for(long boundary=0;boundary<4000 && !success;++boundary)
        {
            FVM_GCRootFrame *roots=c->gc_frames;allowance=boundary;FVM_Result status=fvm_function_invoke(c,fn,0,0,0,&out);allowance=-1;
            assert(c->gc_frames==roots && !c->script_depth && !c->gc_native_depth && !c->avm1_execution_depth && !code->active && !c->execution_mode.native_pool);
            for(fvm_detail::map<fvm_detail::string,FVM_JitCode *>::iterator i=c->execution_mode.cache.begin();i!=c->execution_mode.cache.end();++i)assert(!i->second || !i->second->active);
            if(status==FVM_OK){success=true;assert(out.type==FVM_VALUE_OBJECT && fvm_gc_pin(c,out.object)==FVM_OK);}else{assert(status==FVM_OUT_OF_MEMORY);++failures;}
            assert(fvm_gc_collect(c,0)==FVM_OK);
        }
        assert(success && failures);class_mode=automatic?2:1;FVM_Value result=definition_invoke(c,out.object,body);assert(result.type==FVM_VALUE_NUMBER && result.number==88);
        assert(fvm_function_set_body(c,fn,0,0,0)==FVM_OK && fvm_jit_destroy(code)==FVM_OK);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
