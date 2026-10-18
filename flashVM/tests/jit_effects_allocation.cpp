/* Allocation failures unwind native effects; exceptions retain their values after code release. */
#define main effects_allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
int main()
{
    if(!fvm_jit_available())return 0;
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    const unsigned char ops[]={0x30,0x26,0x34,0x2a};
    for(unsigned loaded=0;loaded<2;++loaded)for(unsigned which=0;which<4;++which)
    {
        FVM_Context *c=0;const unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
        assert(fvm_context_create(rt,&c)==FVM_OK);if(loaded)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
        Program p;if(which!=2)p.text("10");p.op(ops[which]);if(which==1)p.number(42);p.finish();
        FVM_Object *fn=0;FVM_JitCode *code=0;FVM_Value out;
        assert(fvm_function_create_avm1(c,p.bytes.begin(),p.bytes.size(),0,0,0,&fn)==FVM_OK && fvm_gc_pin(c,fn)==FVM_OK);
        assert(fvm_jit_compile(p.bytes.begin(),p.bytes.size(),&code)==FVM_OK && fvm_function_set_body(c,fn,fvm_jit_body,code,0)==FVM_OK);
        bool success=false;unsigned failures=0;
        for(long boundary=0;boundary<1000 && !success;++boundary)
        {
            FVM_GCRootFrame *roots=c->gc_frames;c->random_state=1;allowance=boundary;
            FVM_Result status=fvm_function_invoke(c,fn,0,0,0,&out);allowance=-1;
            assert(c->gc_frames==roots && !c->script_depth && !c->gc_native_depth && !c->avm1_execution_depth && !code->active);
            if(status==(which==3?FVM_EXCEPTION:FVM_OK))
            {
                success=true;
                if(!which)assert(out.type==FVM_VALUE_NUMBER && out.number==8 && c->random_state==1015568748UL);
                if(which==1)assert(out.number==42);
                if(which==2)assert(out.type==FVM_VALUE_NUMBER && out.number>=0);
            }
            else {assert(status==FVM_OUT_OF_MEMORY);++failures;assert(c->random_state==1);assert(fvm_gc_collect(c,0)==FVM_OK);}
        }
        assert(success && failures);
        assert(fvm_function_set_body(c,fn,0,0,0)==FVM_OK && fvm_jit_destroy(code)==FVM_OK && fvm_gc_collect(c,0)==FVM_OK);
        if(which==3)
        {
            assert(fvm_exception_get(c,&out)==FVM_OK && out.type==FVM_VALUE_STRING && !std::strcmp(out.string,"10"));
            assert(fvm_exception_clear(c)==FVM_OK);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
