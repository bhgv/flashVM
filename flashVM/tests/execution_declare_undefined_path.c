/* C89 qualified DefineLocal2 writes existing undefined values, including accessors. */
#include "flashvm/execution.h"
#include "flashvm/standard.h"
#include "flashvm/exception.h"
#include "flashvm/gc.h"
#include "flashvm/function.h"
#include "flashvm/movieclip.h"
#include <assert.h>
static unsigned gets,sets;
static int defined,fail;
static FVM_Result getter(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *raw)
{ (void)frame;(void)raw;++gets;assert(fvm_gc_collect(c,0)==FVM_OK);if(fail){fvm_value_number(out,77);return fvm_exception_raise(c,out);}if(defined==2)return fvm_get_global(c,"clip",out);if(defined)fvm_value_number(out,17);else fvm_value_undefined(out);return FVM_OK; }
static FVM_Result setter(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *raw)
{ (void)c;(void)raw;assert(frame->argc==1 && frame->argv[0].type==FVM_VALUE_UNDEFINED);++sets;fvm_value_undefined(out);return FVM_OK; }
static void accessors(FVM_Context *c,FVM_Object *holder)
{ FVM_Object *get,*put;FVM_Value args[3],out;const unsigned char code[]={0};assert(fvm_function_create_avm1(c,code,sizeof(code),0,0,0,&get)==FVM_OK && fvm_function_set_body(c,get,getter,0,0)==FVM_OK && fvm_function_create_avm1(c,code,sizeof(code),0,0,0,&put)==FVM_OK && fvm_function_set_body(c,put,setter,0,0)==FVM_OK);fvm_value_string(args,"x");fvm_value_undefined(args+1);args[1].type=FVM_VALUE_OBJECT;args[1].object=get;fvm_value_undefined(args+2);args[2].type=FVM_VALUE_OBJECT;args[2].object=put;assert(fvm_object_call(c,holder,"addProperty",3,args,&out)==FVM_OK && out.boolean); }
int main(void)
{
 FVM_Config cfg;FVM_Runtime *rt;unsigned v;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(v=4;v<=8;++v){FVM_Context *c;FVM_Object *holder,*clip;FVM_Value name,value;unsigned before;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)v;assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_construct(c,"Object",0,0,&holder)==FVM_OK);fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=holder;assert(fvm_set_global(c,"holder",&value)==FVM_OK);assert(fvm_movieclip_create(c,"clip",1,&clip)==FVM_OK);value.object=clip;assert(fvm_set_global(c,"clip",&value)==FVM_OK);accessors(c,holder);fvm_value_string(&name,"holder:x");defined=0;before=sets;assert(fvm_exec_variable_declare(c,0,0,0,0,FVM_EXEC_DECLARE_UNINITIALIZED,&name,0)==FVM_OK && sets==before+1);defined=1;before=sets;assert(fvm_exec_variable_declare(c,0,0,0,0,FVM_EXEC_DECLARE_UNINITIALIZED,&name,0)==FVM_OK && sets==before);fail=1;assert(fvm_exec_variable_declare(c,0,0,0,0,FVM_EXEC_DECLARE_UNINITIALIZED,&name,0)==FVM_EXCEPTION && sets==before && fvm_exception_clear(c)==FVM_OK);fail=0;defined=2;before=sets;assert(fvm_exec_variable_declare(c,0,0,0,0,FVM_EXEC_DECLARE_UNINITIALIZED,&name,0)==FVM_OK && sets==before+(v==4?1U:0U));fvm_context_destroy(c);}
 assert(gets==20 && sets==6);fvm_runtime_destroy(rt);return 0;
}
