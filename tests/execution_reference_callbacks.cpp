/* Coercion roots the original scope, explicit receiver and name through GC and exceptions. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include "execution_value_callbacks.cpp"
static const char *reference_text;
static FVM_Result reference_body(FVM_Context *c,const FVM_ScriptFrame *,FVM_Value *out,void *)
{ assert(fvm_gc_collect(c,0)==FVM_OK);if(failing){FVM_Value e;fvm_value_number(&e,77);return fvm_exception_raise(c,&e);}return fvm_value_string(out,reference_text); }
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *scope=0,*key=0,*leaf=0;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
 unsigned char swf[]={'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
 assert(fvm_object_create(c,&scope)==FVM_OK && fvm_object_create(c,&key)==FVM_OK && fvm_object_create(c,&leaf)==FVM_OK);attach(c,key,"toString",reference_body,0);
 FVM_Value name,value,out;fvm_value_undefined(&name);name.type=FVM_VALUE_OBJECT;name.object=key;value=name;value.object=leaf;assert(fvm_object_set(c,scope,"child",&value)==FVM_OK);
 for(unsigned variant=0;variant<2;++variant){reference_text=variant?"this.victim":"child.victim";if(variant){int removed=0;assert(fvm_object_delete(c,scope,"child",&removed)==FVM_OK && removed);}
 fvm_value_number(&value,17);assert(fvm_object_set(c,leaf,"victim",&value)==FVM_OK);
 assert(fvm_exec_delete_reference(c,scope,0,leaf,&name,&out)==FVM_OK && out.boolean);
 assert(fvm_object_set(c,leaf,"victim",&value)==FVM_OK);failing=true;fvm_value_number(&out,99);
 assert(fvm_exec_delete_reference(c,scope,0,leaf,&name,&out)==FVM_EXCEPTION && out.number==99);
 assert(fvm_exception_clear(c)==FVM_OK && fvm_object_get(c,leaf,"victim",&out)==FVM_OK && out.number==17);failing=false;}
 fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
