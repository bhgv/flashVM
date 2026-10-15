/* Malformed records/branches remain atomic; loaded Function2 bounds are checked at invocation. */
#define FVM_DEFINITIONS_FIXTURE_ONLY
#include "jit_definitions.cpp"
static void strict_definition(FVM_Context *c,const Program &p,bool loaded)
{
    if(loaded){FVM_Value out=class_run(c,p);assert(out.type==FVM_VALUE_OBJECT);return;}
    assert(fvm_avm1_execute(c,p.bytes.begin(),p.bytes.size())==FVM_INVALID_ARGUMENT);
    const unsigned char empty[]={0};FVM_Object *fn=0;FVM_JitCode *native=0;FVM_Value out;
    assert(fvm_function_create_avm1(c,empty,sizeof(empty),0,0,0,&fn)==FVM_OK && fvm_gc_pin(c,fn)==FVM_OK && fvm_jit_compile(p.bytes.begin(),p.bytes.size(),&native)==FVM_OK);
    assert(fvm_execution_set_mode(c,FVM_EXECUTION_MANUAL,0)==FVM_OK && fvm_function_set_body(c,fn,fvm_jit_body,native,0)==FVM_OK);
    FVM_GCRootFrame *roots=c->gc_frames;
    assert(fvm_function_invoke(c,fn,0,0,0,&out)==FVM_INVALID_ARGUMENT && c->gc_frames==roots && !native->active && !c->script_depth && !c->avm1_execution_depth);
    assert(fvm_function_set_body(c,fn,0,0,0)==FVM_OK && fvm_jit_destroy(native)==FVM_OK);
}
int main()
{
    if(!fvm_jit_available())return 0;
    const unsigned char good[]={0};FVM_JitCode *code=0,*keep=0;
    assert(fvm_jit_compile(good,sizeof(good),&code)==FVM_OK);keep=code;
    const unsigned char invalid[][12]={{0x9b,0,0,0},{0x9b,5,0,'x',0,0,0,0,0},
        {0x9b,5,0,0,0,0,2,0,0},{0x8e,8,0,0,0,0,0,0,2,0,0,0},
        {0x9b,6,0,0,0,0,1,0,0xff,0}};
    const unsigned sizes[]={4,9,9,12,10};
    for(unsigned i=0;i<sizeof(sizes)/sizeof(sizes[0]);++i)assert(fvm_jit_compile(invalid[i],sizes[i],&code)==FVM_INVALID_ARGUMENT && code==keep);
    Program body;body.number(42);body.finish();Program jump;jump.op(0x99);definition_word(jump,2);definition_word(jump,10);definition_record(jump,"",false,false,body);jump.op(0);
    assert(fvm_jit_compile(jump.bytes.begin(),jump.bytes.size(),&code)==FVM_INVALID_ARGUMENT && code==keep);
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned loaded=0;loaded<2;++loaded)for(class_mode=0;class_mode<3;++class_mode)
    {
        FVM_Context *c=0;const unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
        assert(fvm_context_create(rt,&c)==FVM_OK);if(loaded)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
        if(!loaded && !class_mode)
        {
            Program full;definition_record(full,"",false,false,body);full.op(0);FVM_JitCode *bounded=0;
            assert(fvm_jit_compile(full.bytes.begin(),full.bytes.size(),&bounded)==FVM_OK);
            FVM_ExecutionRoots roots(c,FVM_JIT_MAX_STACK);FVM_JitInvocation state;state.context=c;state.code=bounded;state.stack=&roots.values;
            assert(fvm_jit_define_function(&state,0)==FVM_UNSUPPORTED && roots.values.size()==FVM_JIT_MAX_STACK && fvm_jit_destroy(bounded)==FVM_OK);
        }
        Program strict;definition_record(strict,"",true,false,body,0,1,0);strict.finish();
        strict_definition(c,strict,loaded!=0);
        Program parameter;definition_record(parameter,"",true,true,body,1,0,2);parameter.finish();strict_definition(c,parameter,loaded!=0);
        Program skipped;skipped.op(0x99);definition_word(skipped,2);definition_word(skipped,1);skipped.op(0x08);skipped.number(42);skipped.finish();
        Program parent;definition_record(parent,"",false,false,skipped);parent.finish();FVM_Value out=class_run(c,parent);assert(out.type==FVM_VALUE_OBJECT);
        assert(fvm_execution_set_mode(c,FVM_EXECUTION_JIT,0)==FVM_OK);unsigned long native=c->execution_mode.stats.native_entries;FVM_Value result;
        assert(fvm_function_invoke(c,out.object,0,0,0,&result)==FVM_OK && result.number==42 && c->execution_mode.stats.native_entries==native);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);assert(fvm_jit_destroy(code)==FVM_OK);return 0;
}
