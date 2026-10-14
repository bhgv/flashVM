/* Loaded Extends accepts ordinary classes and defines metadata without inherited setters. */
#include "flashvm/execution.h"
#include "flashvm/standard.h"
#include "flashvm/property.h"
#include "flashvm/gc.h"
#include <assert.h>
static FVM_Result forbidden(FVM_Context *c,FVM_Object *o,unsigned n,const FVM_Value *a,FVM_Value *out,void *raw){(void)c;(void)o;(void)n;(void)a;(void)out;(void)raw;
assert(0);return FVM_ERROR;}
static void obj(FVM_Value *v,FVM_Object *o){fvm_value_undefined(v);v->type=FVM_VALUE_OBJECT;v->object=o;}
int main(void){
FVM_Config cfg;FVM_Runtime *rt;unsigned version;
assert(fvm_config_init(&cfg)==FVM_OK);
assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);for(version=4;version<=8;++version){FVM_Context *c;FVM_Object *sub,*super,*parent,*actual;FVM_Value a,b,v,child;unsigned int flags;int own;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
assert(fvm_context_create(rt,&c)==FVM_OK);
assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
assert(fvm_standard_install(c)==FVM_OK);
assert(fvm_object_create(c,&sub)==FVM_OK);
assert(fvm_gc_pin(c,sub)==FVM_OK);
assert(fvm_object_create(c,&super)==FVM_OK);
assert(fvm_gc_pin(c,super)==FVM_OK);
assert(fvm_object_create(c,&parent)==FVM_OK);obj(&v,parent);
assert(fvm_object_set(c,super,"prototype",&v)==FVM_OK);
assert(fvm_object_define_accessor_rooted(c,parent,"constructor",forbidden,forbidden,0,0)==FVM_OK);
assert(fvm_object_define_accessor_rooted(c,parent,"__constructor__",forbidden,forbidden,0,0)==FVM_OK);obj(&a,sub);obj(&b,super);
assert(fvm_exec_extends(c,&a,&b)==FVM_OK);
assert(fvm_object_get(c,sub,"prototype",&child)==FVM_OK);
assert(child.type==FVM_VALUE_OBJECT);
assert(fvm_object_get_prototype(c,child.object,&actual)==FVM_OK);
assert(actual==parent);
assert(fvm_object_get(c,child.object,"constructor",&v)==FVM_OK);
assert(v.object==super);
assert(fvm_object_get(c,child.object,"__constructor__",&v)==FVM_OK);
assert(v.object==super);
assert(fvm_object_has_own(c,child.object,"constructor",&own)==FVM_OK);
assert(own);
assert(fvm_object_property_flags(c,child.object,"constructor",&flags)==FVM_OK);
assert(flags==FVM_PROPERTY_DONT_ENUM);
assert(fvm_object_property_flags(c,child.object,"__proto__",&flags)==FVM_OK);
assert(flags==FVM_PROPERTY_BUILTIN_FLAGS);
assert(fvm_exec_extends(c,&a,&a)==FVM_OK);
assert(fvm_object_get(c,sub,"prototype",&v)==FVM_OK);
assert(v.object!=child.object);
assert(fvm_object_get_prototype(c,v.object,&actual)==FVM_OK);
assert(actual==child.object);fvm_value_number(&v,17);
assert(fvm_object_set(c,super,"prototype",&v)==FVM_OK);
assert(fvm_exec_extends(c,&a,&b)==FVM_OK);
assert(fvm_object_get(c,sub,"prototype",&child)==FVM_OK);
assert(fvm_object_get_prototype(c,child.object,&actual)==FVM_OK);
assert(!actual);
assert(fvm_object_get(c,child.object,"__proto__",&v)==FVM_OK);
assert(v.type==FVM_VALUE_NUMBER);
assert(v.number==17);
assert(fvm_value_string_bytes(&v,"p\0x",3)==FVM_OK);
assert(fvm_object_set(c,super,"prototype",&v)==FVM_OK);
assert(fvm_exec_extends(c,&a,&b)==FVM_OK);
assert(fvm_gc_collect(c,0)==FVM_OK);
assert(fvm_object_get(c,sub,"prototype",&child)==FVM_OK);
assert(fvm_object_get(c,child.object,"__proto__",&v)==FVM_OK);
assert(v.type==FVM_VALUE_STRING);
assert(fvm_value_string_size(&v)==3);fvm_value_undefined(&b);
assert(fvm_exec_extends(c,&a,&b)==FVM_OK);
assert(fvm_object_get(c,sub,"prototype",&child)==FVM_OK);
assert(fvm_object_get(c,child.object,"__proto__",&v)==FVM_OK);
assert(v.type==FVM_VALUE_UNDEFINED);fvm_context_destroy(c);}fvm_runtime_destroy(rt);return 0;}
