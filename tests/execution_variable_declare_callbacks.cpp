/* Declaration coercion roots its activation and initializer before user code can collect. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include "execution_value_callbacks.cpp"
static FVM_Result key_body(FVM_Context *c,const FVM_ScriptFrame *,FVM_Value *out,void *)
{ assert(fvm_gc_collect(c,0)==FVM_OK);if(failing){FVM_Value e;fvm_value_number(&e,77);return fvm_exception_raise(c,&e);}return fvm_value_string(out,"cell"); }
static unsigned reads;
static FVM_Result read_body(FVM_Context *c,const FVM_ScriptFrame *,FVM_Value *out,void *)
{ ++reads;assert(fvm_gc_collect(c,0)==FVM_OK);if(failing){FVM_Value e;fvm_value_number(&e,77);return fvm_exception_raise(c,&e);}fvm_value_undefined(out);return FVM_OK; }
static FVM_Result forbidden_write(FVM_Context *,const FVM_ScriptFrame *,FVM_Value *,void *)
{ assert(0);return FVM_ERROR; }
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *frame=0,*key=0,*input=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_gc_string_mode(c,FVM_GC_STRINGS_TRACED)==FVM_OK);
 assert(fvm_object_create(c,&frame)==FVM_OK && fvm_object_create(c,&key)==FVM_OK && fvm_object_create(c,&input)==FVM_OK);attach(c,key,"toString",key_body,0);FVM_Value name,value,out;fvm_value_undefined(&name);name.type=FVM_VALUE_OBJECT;name.object=key;fvm_value_number(&value,42);assert(fvm_object_set(c,input,"alive",&value)==FVM_OK);value.type=FVM_VALUE_OBJECT;value.object=input;
 assert(fvm_exec_variable_declare(c,frame,frame,0,0,FVM_EXEC_DECLARE_INITIALIZED,&name,&value)==FVM_OK && fvm_object_get(c,frame,"cell",&out)==FVM_OK && out.object==input && fvm_object_get(c,input,"alive",&out)==FVM_OK && out.number==42);
 failing=true;fvm_value_number(&value,17);assert(fvm_exec_variable_declare(c,frame,frame,0,0,FVM_EXEC_DECLARE_INITIALIZED,&name,&value)==FVM_EXCEPTION && fvm_exception_clear(c)==FVM_OK && fvm_object_get(c,frame,"cell",&out)==FVM_OK && out.object==input);failing=false;
 assert(fvm_exec_variable_declare(c,frame,frame,0,0,FVM_EXEC_DECLARE_UNINITIALIZED,&name,0)==FVM_OK && fvm_object_get(c,frame,"cell",&out)==FVM_OK && out.object==input);
 FVM_Value borrowed;fvm_value_string_bytes(&borrowed,"kept\0text",9);assert(fvm_value_to_string(c,&borrowed,&value)==FVM_OK && fvm_exec_variable_declare(c,frame,frame,0,0,FVM_EXEC_DECLARE_INITIALIZED,&name,&value)==FVM_OK && fvm_object_get(c,frame,"cell",&out)==FVM_OK && fvm_value_string_size(&out)==9 && !std::memcmp(out.string,"kept\0text",9));
 FVM_Object *holder=0,*getter=0,*setter=0;assert(fvm_construct(c,"Object",0,0,&holder)==FVM_OK);const unsigned char empty[]={0};assert(fvm_function_create_avm1(c,empty,sizeof(empty),0,0,0,&getter)==FVM_OK && fvm_function_set_body(c,getter,read_body,0,0)==FVM_OK && fvm_function_create_avm1(c,empty,sizeof(empty),0,0,0,&setter)==FVM_OK && fvm_function_set_body(c,setter,forbidden_write,0,0)==FVM_OK);
 FVM_Value args[3];fvm_value_string(args,"cell");fvm_value_undefined(args+1);args[1].type=FVM_VALUE_OBJECT;args[1].object=getter;fvm_value_undefined(args+2);args[2].type=FVM_VALUE_OBJECT;args[2].object=setter;assert(fvm_object_call(c,holder,"addProperty",3,args,&out)==FVM_OK && out.boolean);value.type=FVM_VALUE_OBJECT;value.object=holder;assert(fvm_set_global(c,"holder",&value)==FVM_OK);fvm_value_string(&name,"holder:cell");assert(fvm_exec_variable_declare(c,0,0,0,0,FVM_EXEC_DECLARE_UNINITIALIZED,&name,0)==FVM_OK && reads==1);
 failing=true;assert(fvm_exec_variable_declare(c,0,0,0,0,FVM_EXEC_DECLARE_UNINITIALIZED,&name,0)==FVM_EXCEPTION && reads==2 && fvm_exception_clear(c)==FVM_OK);failing=false;
 fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
