/* Each native reference invocation allocation boundary restores native/activation/collector state. */
#define main allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#define main jit_reference_fixture_main
#include "jit_references.cpp"
#undef main

int main()
{
    if(!fvm_jit_available())return 0;
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *owner=0,*fn=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);
    assert(fvm_construct(c,"Object",0,0,&owner)==FVM_OK && fvm_gc_pin(c,owner)==FVM_OK);
    Program p;p.text("local");p.number(42);p.op(0x3c);p.text("local");p.op(0x41);
    p.text("copy");p.variable("local");p.op(0x1d);
    p.variable("input");p.text("value");p.variable("copy");p.op(0x4f);
    p.variable("input");p.text("value");p.op(0x3a);p.op(0x17);
    p.text("copy");p.op(0x3b);p.op(0x17);p.variable("local");p.finish();
    const char *parameters[]={"input"};FVM_JitCode *code=0;FVM_Value arg,out;
    assert(fvm_function_create_avm1(c,p.bytes.begin(),p.bytes.size(),1,parameters,0,&fn)==FVM_OK);
    assert(fvm_gc_pin(c,fn)==FVM_OK && fvm_jit_compile(p.bytes.begin(),p.bytes.size(),&code)==FVM_OK);
    assert(fvm_function_set_body(c,fn,fvm_jit_body,code,0)==FVM_OK);
    fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=owner;
    bool success=false;unsigned failures=0;
    for(long boundary=0;boundary<2000 && !success;++boundary)
    {
        FVM_GCRootFrame *roots=c->gc_frames;allowance=boundary;
        FVM_Result status=fvm_function_invoke(c,fn,0,1,&arg,&out);allowance=-1;
        assert(c->gc_frames==roots && !c->gc_native_depth && !c->script_depth &&
               !c->avm1_execution_depth && !code->active);
        if(status==FVM_OK){assert(out.type==FVM_VALUE_NUMBER && out.number==42);success=true;}
        else{assert(status==FVM_OUT_OF_MEMORY);++failures;}
        assert(fvm_gc_collect(c,0)==FVM_OK);
    }
    assert(success && failures && fvm_function_set_body(c,fn,0,0,0)==FVM_OK && fvm_jit_destroy(code)==FVM_OK);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
