/* Enumeration failure injection and exact stack bounds preserve roots and name publication. */
#define main enum_allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#define FVM_ENUMERATE_FIXTURE_ONLY
#include "jit_enumerate.cpp"
int main()
{
    if(!fvm_jit_available())return 0;
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned loaded=0;loaded<2;++loaded)for(unsigned named=0;named<2;++named)
    {
        FVM_Context *c=0;const unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
        assert(fvm_context_create(rt,&c)==FVM_OK);if(loaded)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
        FVM_Object *object=enum_setup(c);FVM_Value value=class_value(object),expected,out;
        assert(fvm_exec_enumerate(c,&value,&expected)==FVM_OK && fvm_gc_pin(c,expected.object)==FVM_OK);
        Program p=enum_program(named!=0,3);FVM_Object *fn=0;FVM_JitCode *code=0;
        assert(fvm_function_create_avm1(c,p.bytes.begin(),p.bytes.size(),0,0,0,&fn)==FVM_OK && fvm_gc_pin(c,fn)==FVM_OK);
        assert(fvm_jit_compile(p.bytes.begin(),p.bytes.size(),&code)==FVM_OK && fvm_function_set_body(c,fn,fvm_jit_body,code,0)==FVM_OK);
        bool success=false;unsigned failures=0;
        for(long boundary=0;boundary<3000 && !success;++boundary)
        {
            FVM_GCRootFrame *roots=c->gc_frames;allowance=boundary;
            FVM_Result status=fvm_function_invoke(c,fn,0,0,0,&out);allowance=-1;
            assert(c->gc_frames==roots && !c->script_depth && !c->gc_native_depth && !c->avm1_execution_depth && !code->active);
            if(status==FVM_OK){success=true;enum_result(c,out,expected);assert(fvm_gc_pin(c,out.object)==FVM_OK);}
            else {assert(status==FVM_OUT_OF_MEMORY);++failures;assert(fvm_gc_collect(c,0)==FVM_OK);}
        }
        assert(success && failures && fvm_function_set_body(c,fn,0,0,0)==FVM_OK && fvm_jit_destroy(code)==FVM_OK);
        assert(fvm_gc_collect(c,0)==FVM_OK);enum_result(c,out,expected);
        {
            FVM_ExecutionRoots operands(c,FVM_JIT_MAX_STACK);operands.values.back()=value;
            FVM_JitInvocation state={0,0,0,0,0,FVM_Value(),0,false};state.context=c;state.stack=&operands.values;
            FVM_GCRootFrame *roots=c->gc_frames;
            assert(fvm_jit_safe<fvm_jit_enumerate>(&state,0)==FVM_UNSUPPORTED && c->gc_frames==roots);
            assert(operands.values.size()==FVM_JIT_MAX_STACK && operands.values.back().object==object);
            operands.values.resize(FVM_JIT_MAX_STACK-3);operands.values.back()=value;
            assert(fvm_jit_safe<fvm_jit_enumerate>(&state,0)==FVM_OK && c->gc_frames==roots && operands.values.size()==FVM_JIT_MAX_STACK);
            assert(fvm_gc_collect(c,0)==FVM_OK && operands.values.back().type==FVM_VALUE_STRING);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
