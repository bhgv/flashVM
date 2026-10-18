/* Loaded catch names perform persistent variable assignments, including With and qualified paths. */
#define main exceptions_fixture_main
#include "actionscript_exceptions.cpp"
#undef main
#include "flashvm/function.h"
#include "flashvm/standard.h"
#include "../src/core/runtime.hpp"
static unsigned setters;
static FVM_Object *expected_receiver;
static FVM_Result forbidden(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *,void *)
{ assert(0);return FVM_ERROR; }
static FVM_Result write_slot(FVM_Context *,FVM_Object *self,unsigned n,const FVM_Value *args,FVM_Value *out,void *)
{ assert(self==expected_receiver && n==1 && args[0].number==42);++setters;fvm_value_undefined(out);return FVM_OK; }
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=4;version<=8;++version){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);FVM_Value value,out;
 fvm_value_number(&value,17);assert(fvm_set_global(c,"error",&value)==FVM_OK);Bytes body,caught,final,code,empty;integer(body,42);body.push_back(0x2a);string(caught,"observed");variable(caught,"error");caught.push_back(0x1d);string(final,"after");variable(final,"error");final.push_back(0x1d);attempt(code,body,caught,final,3,"error");run(c,code,FVM_OK);
 assert(global(c,"observed")==42 && global(c,"after")==42 && global(c,"error")==42);
 Bytes local;string(local,"error");integer(local,17);local.push_back(0x3c);attempt(local,body,empty,empty,1,"error");variable(local,"error");local.push_back(0x3e);local.push_back(0);FVM_Object *fn=0;assert(fvm_function_create_avm1(c,local.begin(),local.size(),0,0,0,&fn)==FVM_OK && fvm_function_invoke(c,fn,0,0,0,&out)==FVM_OK && out.number==42 && global(c,"error")==42);
 Bytes fresh;attempt(fresh,body,empty,empty,1,"freshError");variable(fresh,"freshError");fresh.push_back(0x3e);fresh.push_back(0);assert(fvm_function_create_avm1(c,fresh.begin(),fresh.size(),0,0,0,&fn)==FVM_OK && fvm_function_invoke(c,fn,0,0,0,&out)==FVM_OK && out.number==42 && global(c,"freshError")==42);
 FVM_Object *holder=0;assert(fvm_construct(c,"Object",0,0,&holder)==FVM_OK);fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=holder;assert(fvm_set_global(c,"holder",&value)==FVM_OK);Bytes qualified,read;string(read,"qualified");variable(read,"holder:slot");read.push_back(0x1d);attempt(qualified,body,read,empty,1,"holder:slot");run(c,qualified,FVM_OK);assert(global(c,"qualified")==42 && fvm_object_get(c,holder,"slot",&out)==FVM_OK && out.number==42);
 assert(fvm_get_global(c,"holder:slot",&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
 expected_receiver=holder;assert(fvm_object_define_accessor_rooted(c,holder,"native",forbidden,write_slot,0,0)==FVM_OK);Bytes attempt_body,with;attempt(attempt_body,body,empty,empty,1,"native");variable(with,"holder");with.push_back(0x94);word(with,2);word(with,attempt_body.size());append(with,attempt_body);run(c,with,FVM_OK);
 assert(c->lexical_scopes.empty() && !c->gc_frames && !c->gc_native_depth && !c->script_depth);fvm_context_destroy(c);}
 assert(setters==5);fvm_runtime_destroy(rt);return 0;
}
