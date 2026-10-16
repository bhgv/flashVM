/* Qualified CallFunction retains lookup roots while invoking its value at superclass depth zero. */
#define main super_fixture_main
#include "avm1_super.cpp"
#undef main
#include "flashvm/execution.h"
#include "flashvm/extensions.h"
#include "flashvm/property.h"
#include "../src/core/runtime.hpp"
static FVM_Result detached_getter(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *)
{
 int removed=0;FVM_Value missing;fvm_value_undefined(&missing);assert(fvm_set_global(c,"Base",&missing)==FVM_OK && fvm_set_global(c,"Sub",&missing)==FVM_OK && fvm_set_global(c,"Third",&missing)==FVM_OK && fvm_object_delete(c,frame->receiver->prototype->prototype,"foo",&removed)==FVM_OK && removed && fvm_gc_collect(c,0)==FVM_OK);fvm_value_undefined(out);out->type=FVM_VALUE_OBJECT;out->object=frame->capture;return FVM_OK;
}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_extensions_install(c)==FVM_OK);
 Bytes base_body;variable(base_body,"this");text(base_body,"marker");integer(base_body,123);base_body.push_back(0x4f);base_body.push_back(0);FVM_Value base=define(c,"Base",0,32,base_body);
 Bytes child_body;integer(child_body,0);variable(child_body,"super");text(child_body,"");child_body.push_back(0x52);child_body.push_back(0x17);child_body.push_back(0);FVM_Value sub=define(c,"Sub",0,0,child_body);extend(c,"Sub","Base");
 Bytes read_body;variable(read_body,"this");text(read_body,"marker");read_body.push_back(0x4e);result(read_body);FVM_Value read=define(c,"read",0,32,read_body);assert(fvm_object_set(c,prototype(c,base),"read",&read)==FVM_OK);
 Bytes foo_body;integer(foo_body,0);variable(foo_body,"gc");text(foo_body,"collect");foo_body.push_back(0x52);foo_body.push_back(0x17);integer(foo_body,0);reg(foo_body,1);text(foo_body,"read");foo_body.push_back(0x52);result(foo_body);FVM_Value foo=define(c,"foo",2,16,foo_body);FVM_Object *sub_proto=prototype(c,sub);assert(fvm_object_set(c,sub_proto,"foo",&foo)==FVM_OK);
 Bytes override_body;integer(override_body,900);result(override_body);FVM_Value over=define(c,"overrideRead",0,32,override_body);assert(fvm_object_set(c,sub_proto,"read",&over)==FVM_OK);FVM_Value third=define(c,"Third",0,0,child_body);extend(c,"Third","Sub");FVM_Object *instance=0;assert(fvm_function_construct(c,third.object,0,0,&instance)==FVM_OK);FVM_Value value,out,name;fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=instance;assert(fvm_set_global(c,"object",&value)==FVM_OK);fvm_value_string(&name,"object:foo");assert(fvm_exec_call_function(c,0,0,0,0,0,&name,0,0,&out)==FVM_OK && out.number==900);
 const unsigned char empty[]={0};FVM_Object *getter_fn=0;assert(fvm_function_create_avm1(c,empty,sizeof(empty),0,0,0,&getter_fn)==FVM_OK && fvm_function_set_body(c,getter_fn,detached_getter,0,foo.object)==FVM_OK);FVM_Value args[3];fvm_value_string(args,"dynamicFoo");fvm_value_undefined(args+1);args[1].type=FVM_VALUE_OBJECT;args[1].object=getter_fn;fvm_value_undefined(args+2);args[2].type=FVM_VALUE_NULL;assert(fvm_object_call(c,sub_proto,"addProperty",3,args,&out)==FVM_OK && out.boolean);
 fvm_value_string(&name,"object:dynamicFoo");assert(fvm_exec_call_function(c,0,0,0,0,0,&name,0,0,&out)==FVM_OK && out.number==900 && !c->calling_function && !c->calling_owner && !c->calling_constructor && !c->gc_frames && fvm_gc_collect(c,0)==FVM_OK);
 fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
