/* Native request dispatch contains compile/invoke OOM, including automatic nested frame compilation. */
#define main dispatch_allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#define FVM_FRAME_DISPATCH_FIXTURE_ONLY
#include "jit_frame_dispatch.cpp"
int main()
{
    if(!fvm_jit_available())return 0;
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned loaded=0;loaded<2;++loaded)for(unsigned kind=0;kind<4;++kind)
    {
        FVM_Context *c=0;const unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
        assert(fvm_context_create(rt,&c)==FVM_OK);if(loaded)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
        assert(fvm_standard_install(c)==FVM_OK && fvm_avm1_set_url_rooted(c,dispatch_sink,0,0)==FVM_OK);
        FVM_Object *root=0;if(kind>=2){assert(fvm_movieclip_create(c,"root",2,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);frame_define(c,root,2,"second",42);}
        Program p;p.number(77);if(!kind)dispatch_literal(p,"url","target");if(kind==1){p.text("url");p.text("target");dispatch_dynamic(p,194);}if(kind>=2){p.text("second");dispatch_call(p);}p.finish();
        FVM_Object *fn=0;FVM_JitCode *code=0;FVM_Value out;
        assert(fvm_function_create_avm1(c,p.bytes.begin(),p.bytes.size(),0,0,0,&fn)==FVM_OK && fvm_gc_pin(c,fn)==FVM_OK && fvm_jit_compile(p.bytes.begin(),p.bytes.size(),&code)==FVM_OK);
        bool success=false;unsigned failures=0;
        for(long boundary=0;boundary<2000 && !success;++boundary)
        {
            FVM_JitCode *candidate=code;allowance=boundary;FVM_Result status=fvm_jit_compile(p.bytes.begin(),p.bytes.size(),&candidate);allowance=-1;
            if(status==FVM_OK){success=true;assert(candidate!=code && fvm_jit_destroy(candidate)==FVM_OK);}else{assert(status==FVM_OUT_OF_MEMORY && candidate==code);++failures;}
        }
        assert(success && failures);success=false;failures=0;
        assert(fvm_execution_set_mode(c,kind==3?FVM_EXECUTION_JIT:FVM_EXECUTION_MANUAL,0)==FVM_OK);
        if(kind!=3)assert(fvm_function_set_body(c,fn,fvm_jit_body,code,0)==FVM_OK);
        for(long boundary=0;boundary<3000 && !success;++boundary)
        {
            FVM_GCRootFrame *roots=c->gc_frames;allowance=boundary;FVM_Result status=fvm_function_invoke(c,fn,root,0,0,&out);allowance=-1;
            assert(c->gc_frames==roots && !c->script_depth && !c->gc_native_depth && !c->avm1_execution_depth && !code->active);
            if(status==FVM_OK){success=true;assert(out.type==FVM_VALUE_NUMBER && out.number==77);if(root)frame_seen(c,root,42);}
            else{assert(status==FVM_OUT_OF_MEMORY);++failures;}
            assert(fvm_gc_collect(c,0)==FVM_OK);
        }
        assert(success && failures && fvm_execution_set_mode(c,FVM_EXECUTION_MANUAL,0)==FVM_OK && fvm_function_set_body(c,fn,0,0,0)==FVM_OK && fvm_jit_destroy(code)==FVM_OK);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
