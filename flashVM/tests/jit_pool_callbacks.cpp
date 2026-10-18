/* Nested native functions/root entry restore pool snapshots through getter GC and exceptions. */
#include "../src/jit/internal.hpp"
#include "../src/standard/function_state.hpp"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include <cassert>
#include <cstring>

struct Nested { FVM_Object *inner; bool throwing; };
static FVM_Result getter(FVM_Context *c,const FVM_ScriptFrame *,FVM_Value *out,void *raw)
{
    Nested *state=(Nested *)raw;FVM_Value value;
    const fvm_detail::vector<fvm_detail::string> *pool=c->execution_mode.native_pool;
    assert(pool && pool->size()==1 && (*pool)[0]=="getter");
    assert(fvm_function_invoke(c,state->inner,0,0,0,&value)==FVM_OK &&
           value.type==FVM_VALUE_STRING && !std::strcmp(value.string,"inner"));
    assert(c->execution_mode.native_pool==pool);
    const unsigned char root[]={0x96,2,0,8,0,0x3e,0};FVM_ExecutionOptions options;
    assert(fvm_execution_options_init(&options)==FVM_OK);options.mode=FVM_EXECUTION_JIT;
    assert(fvm_avm1_execute_options(c,root,sizeof(root),&options)==
           (c->swf_loaded?FVM_OK:FVM_INVALID_ARGUMENT));
    assert(c->execution_mode.native_pool==pool && fvm_gc_collect(c,0)==FVM_OK);
    fvm_value_number(out,17);return state->throwing?fvm_exception_raise(c,out):FVM_OK;
}
static FVM_Object *make(FVM_Context *c,const unsigned char *bytes,size_t size,const char *pool,
                       const char *parameter=0)
{
    FVM_ScriptFunction definition;
    definition.body.assign(bytes,bytes+size);definition.constants.push_back(fvm_detail::string(pool));
    if(parameter){definition.parameters.push_back(fvm_detail::string(parameter));definition.parameter_registers.push_back(0);}
    FVM_Object *fn=0;assert(fvm_function_make_script(c,definition,0,&fn)==FVM_OK && fvm_gc_pin(c,fn)==FVM_OK);
    return fn;
}
int main()
{
    if(!fvm_jit_available())return 0;
    const unsigned char inner_body[]={0x96,2,0,8,0,0x3e,0};
    const unsigned char outer_body[]={0x88,11,0,1,0,'s','e','l','e','c','t','e','d',0,
        0x96,6,0,0,'g','a','t','e',0,0x1c,0x96,8,0,0,'i','n','v','o','k','e',0,0x4e,0x17,
        0x96,2,0,8,0,0x3e,0};
    const unsigned char empty[]={0};FVM_Config cfg;FVM_Runtime *rt=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=0;version<=8;++version)
    {
        if(version && version<4)continue;
        FVM_Context *c=0;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
        assert(fvm_context_create(rt,&c)==FVM_OK);
        if(version){swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);}
        assert(fvm_standard_install(c)==FVM_OK && fvm_gc_string_mode(c,FVM_GC_STRINGS_TRACED)==FVM_OK);
        FVM_Object *inner=make(c,inner_body,sizeof(inner_body),"inner");
        FVM_Object *outer=make(c,outer_body,sizeof(outer_body),"captured","gate");
        FVM_Object *callback=make(c,empty,sizeof(empty),"getter");FVM_Object *setter=make(c,empty,sizeof(empty),"setter");
        FVM_Object *owner=0;FVM_JitCode *inner_code=0,*outer_code=0;FVM_Value args[3],out,arg;
        assert(fvm_construct(c,"Object",0,0,&owner)==FVM_OK && fvm_gc_pin(c,owner)==FVM_OK);
        assert(fvm_jit_compile(inner_body,sizeof(inner_body),&inner_code)==FVM_OK &&
               fvm_jit_compile(outer_body,sizeof(outer_body),&outer_code)==FVM_OK);
        assert(fvm_function_set_body(c,inner,fvm_jit_body,inner_code,0)==FVM_OK &&
               fvm_function_set_body(c,outer,fvm_jit_body,outer_code,0)==FVM_OK);
        Nested state={inner,false};assert(fvm_function_set_body(c,callback,getter,&state,0)==FVM_OK);
        fvm_value_string(args,"invoke");fvm_value_undefined(args+1);args[1].type=FVM_VALUE_OBJECT;args[1].object=callback;
        fvm_value_undefined(args+2);args[2].type=FVM_VALUE_OBJECT;args[2].object=setter;
        assert(fvm_object_call(c,owner,"addProperty",3,args,&out)==FVM_OK && out.boolean);
        fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=owner;
        for(unsigned throwing=0;throwing<2;++throwing)
        {
            state.throwing=throwing!=0;assert(fvm_execution_set_mode(c,FVM_EXECUTION_MANUAL,0)==FVM_OK);
            unsigned long before=c->execution_mode.stats.native_entries;FVM_GCRootFrame *roots=c->gc_frames;
            FVM_Result status=fvm_function_invoke(c,outer,0,1,&arg,&out);
            assert(c->execution_mode.stats.native_entries==before+3 && !c->execution_mode.native_pool &&
                   c->gc_frames==roots && !c->script_depth && !c->avm1_execution_depth &&
                   !inner_code->active && !outer_code->active);
            if(throwing)assert(status==FVM_EXCEPTION && fvm_exception_clear(c)==FVM_OK);
            else assert(status==FVM_OK && out.type==FVM_VALUE_STRING && !std::strcmp(out.string,"selected") &&
                        fvm_set_global(c,"retained",&out)==FVM_OK);
        }
        assert(fvm_function_set_body(c,inner,0,0,0)==FVM_OK && fvm_function_set_body(c,outer,0,0,0)==FVM_OK);
        assert(fvm_jit_destroy(inner_code)==FVM_OK && fvm_jit_destroy(outer_code)==FVM_OK && fvm_gc_collect(c,0)==FVM_OK);
        assert(fvm_get_global(c,"retained",&out)==FVM_OK && out.type==FVM_VALUE_STRING && !std::strcmp(out.string,"selected"));
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
