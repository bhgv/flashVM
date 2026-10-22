/* Shared owned-field fixture for paragraph inset contracts. */
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include "textfield_counted_text.cpp"
static void inset_number(FVM_Context *c,FVM_Object *o,const char *name,double n)
{FVM_Value v;fvm_value_number(&v,n);assert(fvm_object_set(c,o,name,&v)==FVM_OK);}
static void inset_string(FVM_Context *c,FVM_Object *o,const char *name,const char *s)
{FVM_Value v;assert(fvm_value_string(&v,s)==FVM_OK && fvm_object_set(c,o,name,&v)==FVM_OK);}
static void inset_bool(FVM_Context *c,FVM_Object *o,const char *name,bool b)
{FVM_Value v;fvm_value_boolean(&v,b);assert(fvm_object_set(c,o,name,&v)==FVM_OK);}
static void inset_apply(FVM_Context *c,FVM_Object *field,FVM_Object *format,const char *method)
{FVM_Value v,out;fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=format;assert(fvm_object_call(c,field,method,1,&v,&out)==FVM_OK);}
static FVM_Object *inset_format(FVM_Context *c)
{FVM_Object *o=0;assert(fvm_construct(c,"TextFormat",0,0,&o)==FVM_OK && fvm_gc_pin(c,o)==FVM_OK);inset_number(c,o,"size",40);inset_number(c,o,"leftMargin",10);inset_number(c,o,"rightMargin",12);inset_number(c,o,"blockIndent",8);return o;}
#ifndef FVM_INSETS_FIXTURE_ONLY
int main(){return 0;}
#endif
