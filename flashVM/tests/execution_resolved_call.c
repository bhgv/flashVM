/* C89 resolved dispatch preserves binding presence, counted fallback names and aliasing. */
#include "flashvm/execution.h"
#include "flashvm/function.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include <assert.h>
static unsigned calls;static int fail;
static FVM_Result callback(FVM_Context *c,FVM_Object *self,unsigned n,const FVM_Value *args,FVM_Value *out,void *raw)
{ (void)c;if(raw)assert(self==(FVM_Object *)raw);assert(n==1 && args[0].number==17);++calls;fvm_value_number(out,23);return FVM_OK; }
static FVM_Result body(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *raw)
{ (void)raw;assert(frame->argc==1 && frame->argv[0].number==17);assert(fvm_gc_collect(c,0)==FVM_OK);if(fail){FVM_Value e;fvm_value_number(&e,77);return fvm_exception_raise(c,&e);}fvm_value_number(out,31);return FVM_OK; }
int main(void)
{
    FVM_Config cfg;FVM_Runtime *rt;FVM_Context *c;FVM_Object *o,*fn;
    FVM_Value name,callable,arg,out;const unsigned char code[]={0};
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);
    assert(fvm_object_create(c,&o)==FVM_OK && fvm_gc_pin(c,o)==FVM_OK);
    assert(fvm_function_create_native(c,callback,o,0,&fn)==FVM_OK);
    fvm_value_undefined(&callable);callable.type=FVM_VALUE_OBJECT;callable.object=fn;
    fvm_value_string_bytes(&name,"a\0b",3);fvm_value_number(&arg,17);
    assert(fvm_exec_resolved_call(c,0,&name,&callable,1,o,1,&arg,&callable)==FVM_OK && callable.number==23 && calls==1);
    fvm_value_undefined(&callable);fvm_value_number(&out,99);
    assert(fvm_exec_resolved_call(c,0,&name,&callable,1,o,1,&arg,&out)==FVM_UNSUPPORTED && out.number==99);
    assert(fvm_register_function_rooted_bytes(c,"a\0b",3,callback,0,0)==FVM_OK);
    assert(fvm_exec_resolved_call(c,0,&name,&callable,0,o,1,&arg,&out)==FVM_OK && out.number==23 && calls==2);
    assert(fvm_function_create_avm1(c,code,sizeof(code),0,0,0,&fn)==FVM_OK && fvm_function_set_body(c,fn,body,0,0)==FVM_OK);
    callable.type=FVM_VALUE_OBJECT;callable.object=fn;assert(fvm_gc_pin(c,fn)==FVM_OK);
    assert(fvm_exec_resolved_call(c,0,&name,&callable,1,o,1,&arg,&out)==FVM_OK && out.number==31);
    assert(fvm_exec_resolved_call(c,1,&name,&callable,1,0,1,&arg,&out)==FVM_OK && out.type==FVM_VALUE_OBJECT);
    fail=1;fvm_value_number(&out,99);
    assert(fvm_exec_resolved_call(c,0,&name,&callable,1,o,1,&arg,&out)==FVM_EXCEPTION && out.number==99 && fvm_exception_clear(c)==FVM_OK);fail=0;
    fvm_value_undefined(&callable);fvm_value_string(&name,"Object");
    assert(fvm_exec_resolved_call(c,1,&name,&callable,0,0,0,0,&out)==FVM_OK && out.type==FVM_VALUE_OBJECT);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
