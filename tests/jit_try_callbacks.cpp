/* Suspended baseline/return/exception graphs and rich faults survive collecting finally bodies. */
#define FVM_TRY_FIXTURE_ONLY
#include "jit_try.cpp"
#include "../src/core/exception_internal.hpp"
static FVM_Object *payload;
static FVM_JitCode *explicit_code;
static unsigned collections,scenario;
static void try_active(FVM_Context *c)
{
    if(!class_mode)return;FVM_JitCode *code=explicit_code;
    if(class_mode==2){code=0;for(fvm_detail::map<fvm_detail::string,FVM_JitCode *>::iterator i=c->execution_mode.cache.begin();i!=c->execution_mode.cache.end();++i)if(i->second && i->second->active && i->second->blocks.size()==3)code=i->second;}
    assert(code && code->active && fvm_jit_destroy(code)==FVM_INVALID_ARGUMENT);unsigned active=0;for(size_t i=0;i<code->blocks.size();++i)if(code->blocks[i].code->active){++active;assert(fvm_jit_destroy(code->blocks[i].code)==FVM_INVALID_ARGUMENT);}assert(active==1);
}
static FVM_Result try_collect(FVM_Context *c,const FVM_ScriptFrame *,FVM_Value *out,void *)
{
    ++collections;assert(!c->exception_pending && !c->fault_pending && fvm_gc_collect(c,0)==FVM_OK);try_active(c);FVM_Value value;assert(fvm_object_get(c,payload,"alive",&value)==FVM_OK && value.number==17);fvm_value_undefined(out);return FVM_OK;
}
static FVM_Result try_fault(FVM_Context *c,const FVM_ScriptFrame *,FVM_Value *out,void *)
{
    fvm_resource_fault(c,FVM_RESOURCE_FAILURE,99,"try-test","synthetic");assert(fvm_exception_get(c,out)==FVM_OK && out->type==FVM_VALUE_OBJECT);payload=out->object;FVM_Value value;fvm_value_number(&value,17);assert(fvm_object_set(c,payload,"alive",&value)==FVM_OK);return FVM_EXCEPTION;
}
static FVM_Object *try_callback(FVM_Context *c,FVM_ScriptBody body)
{const unsigned char empty[]={0};FVM_Object *fn=0;assert(fvm_function_create_avm1(c,empty,sizeof(empty),0,0,0,&fn)==FVM_OK && fvm_function_set_body(c,fn,body,0,0)==FVM_OK);return fn;}
static void try_invoke_name(Program &p,const char *name)
{p.number(0);p.text(name);p.op(0x3d);p.op(0x17);}
static void try_forget(Program &p)
{p.text("payload");p.op(0x96);definition_word(p,1);p.op(3);p.op(0x1d);}
int main()
{
    if(!fvm_jit_available())return 0;FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=0;version<=8;++version)for(class_mode=0;class_mode<3;++class_mode)for(scenario=0;scenario<7;++scenario)
    {
        if((version && version<4) || (scenario==6 && version))continue;FVM_Context *c=0;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_context_create(rt,&c)==FVM_OK);if(version)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);assert(fvm_standard_install(c)==FVM_OK && fvm_gc_string_mode(c,FVM_GC_STRINGS_TRACED)==FVM_OK);
        FVM_Value value,out;assert(fvm_construct(c,"Object",0,0,&payload)==FVM_OK);fvm_value_number(&value,17);assert(fvm_object_set(c,payload,"alive",&value)==FVM_OK);value=class_value(payload);assert(fvm_set_global(c,"payload",&value)==FVM_OK);
        value=class_value(try_callback(c,try_collect));assert(fvm_set_global(c,"collect",&value)==FVM_OK);value=class_value(try_callback(c,try_fault));assert(fvm_set_global(c,"fault",&value)==FVM_OK);
        Program body,caught,final,p;unsigned flags=2;
        if(scenario==6){p.text("e");p.variable("payload");p.op(0x3c);try_forget(p);body=try_exit(42,true);try_invoke_name(caught,"collect");flags=1;}
        else if(scenario==2){p.variable("payload");body.op(0x17);try_forget(body);try_invoke_name(body,"collect");body.number(42);body.op(0x2a);flags=1;}
        else if(scenario==4){try_forget(body);try_invoke_name(body,"fault");try_invoke_name(final,"collect");}
        else
        {
            body.variable("payload");try_forget(body);body.op(scenario==1 || scenario==3?0x2a:0x3e);
            if(scenario==3){try_invoke_name(caught,"collect");caught.variable("e");caught.op(0x3e);flags=3;}
            if(scenario!=1)final.op(0x17);try_invoke_name(final,"collect");if(scenario==5){final.number(23);final.op(0x2a);}
        }
        try_block(p,body,caught,final,flags);if(scenario==6)p.variable("e");p.finish();FVM_Object *fn=0;explicit_code=0;
        assert(fvm_function_create_avm1(c,p.bytes.begin(),p.bytes.size(),0,0,0,&fn)==FVM_OK && fvm_gc_pin(c,fn)==FVM_OK && fvm_jit_compile(p.bytes.begin(),p.bytes.size(),&explicit_code)==FVM_OK && fvm_execution_set_mode(c,class_mode==2?FVM_EXECUTION_JIT:FVM_EXECUTION_MANUAL,0)==FVM_OK);if(class_mode==1)assert(fvm_function_set_body(c,fn,fvm_jit_body,explicit_code,0)==FVM_OK);
        collections=0;FVM_GCRootFrame *roots=c->gc_frames;bool raised=scenario==1 || scenario==4 || scenario==5;
        assert(fvm_function_invoke(c,fn,0,0,0,&out)==(raised?FVM_EXCEPTION:FVM_OK));assert(collections==(scenario==3?2U:1U) && c->gc_frames==roots && c->lexical_scopes.empty() && !c->script_depth && !c->avm1_execution_depth && !c->gc_native_depth && !c->execution_mode.native_pool && !explicit_code->active);
        if(scenario==4){FVM_ExceptionInfo info;assert(fvm_exception_info(c,&info)==FVM_OK && info.code==FVM_RESOURCE_FAILURE && info.resource==99 && !std::strcmp(info.operation,"try-test") && !std::strcmp(info.address,"synthetic"));}
        if(raised)assert(fvm_exception_get(c,&out)==FVM_OK);
        if(scenario==5)assert(out.type==FVM_VALUE_NUMBER && out.number==23);else assert(out.type==FVM_VALUE_OBJECT && out.object==payload);
        if(out.type==FVM_VALUE_OBJECT)assert(fvm_gc_pin(c,out.object)==FVM_OK);assert(fvm_exception_clear(c)==FVM_OK && fvm_execution_set_mode(c,FVM_EXECUTION_MANUAL,0)==FVM_OK && fvm_function_set_body(c,fn,0,0,0)==FVM_OK && fvm_jit_destroy(explicit_code)==FVM_OK && fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
