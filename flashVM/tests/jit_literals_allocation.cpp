/* Native literal materialization restores roots and active state at each failing allocation. */
#define main literal_allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#define FVM_LITERAL_CALLBACK_FIXTURE_ONLY
#include "jit_literals_callbacks.cpp"
int main()
{
    if(!fvm_jit_available())return 0;
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned loaded=0;loaded<2;++loaded)for(unsigned object=0;object<2;++object)
    {
        FVM_Context *c=0;FVM_Object *fn=0;FVM_JitCode *code=0;FVM_Value out;
        const unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
        assert(fvm_context_create(rt,&c)==FVM_OK);if(loaded)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
        assert(fvm_standard_install(c)==FVM_OK);FVM_Value arg=coercible(c,!object);
        Program p;if(object)p.variable("input");p.number(42);
        if(!object && loaded)p.variable("input");else p.number(1);
        p.op(object?0x43:0x42);p.finish();const char *params[]={"input"};
        assert(fvm_function_create_avm1(c,p.bytes.begin(),p.bytes.size(),1,params,0,&fn)==FVM_OK);
        assert(fvm_gc_pin(c,fn)==FVM_OK && fvm_jit_compile(p.bytes.begin(),p.bytes.size(),&code)==FVM_OK);
        assert(fvm_function_set_body(c,fn,fvm_jit_body,code,0)==FVM_OK);
        bool success=false;unsigned failures=0;
        for(long boundary=0;boundary<3000 && !success;++boundary)
        {
            FVM_GCRootFrame *roots=c->gc_frames;allowance=boundary;
            FVM_Result status=fvm_function_invoke(c,fn,0,1,&arg,&out);allowance=-1;
            assert(c->gc_frames==roots && !c->script_depth && !c->gc_native_depth && !code->active);
            if(status==FVM_OK){success=true;assert(out.type==FVM_VALUE_OBJECT);}
            else{assert(status==FVM_OUT_OF_MEMORY);++failures;assert(fvm_gc_collect(c,0)==FVM_OK);}
        }
        FVM_GCRootScope *hold=0;assert(success && failures && fvm_gc_roots_begin(c,&hold)==FVM_OK);
        assert(fvm_gc_roots_add(hold,&out)==FVM_OK && fvm_function_set_body(c,fn,0,0,0)==FVM_OK);
        assert(fvm_jit_destroy(code)==FVM_OK && fvm_gc_collect(c,0)==FVM_OK);
        FVM_Value value;assert(fvm_object_get(c,out.object,object?"key":"0",&value)==FVM_OK && value.type==FVM_VALUE_NUMBER && value.number==42);
        assert(fvm_gc_roots_end(hold)==FVM_OK);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
