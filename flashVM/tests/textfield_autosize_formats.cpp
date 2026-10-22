/* Current/future formatting, kerning, tracking, underline, NUL and empty insertion fit content. */
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include "textfield_counted_text.cpp"
#include <cmath>
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
 FVM_Object *field=counted_field(c),*format=0;FVM_Value value,arg,out,args[3];
 assert(fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK);fvm_value_number(&value,40);assert(fvm_object_set(c,format,"size",&value)==FVM_OK);
 fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=format;assert(fvm_object_call(c,field,"setNewTextFormat",1,&arg,&out)==FVM_OK);
 assert(fvm_value_string(&value,"AV")==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK);fvm_value_boolean(&value,1);assert(fvm_object_set(c,field,"autoSize",&value)==FVM_OK);
 assert(fvm_object_set(c,format,"kerning",&value)==FVM_OK && fvm_object_call(c,field,"setTextFormat",1,&arg,&out)==FVM_OK && std::fabs(counted_metric(c,field,"_width")-48.8)<0.02);
 fvm_value_number(&value,2.5);assert(fvm_object_set(c,format,"letterSpacing",&value)==FVM_OK && fvm_object_call(c,field,"setTextFormat",1,&arg,&out)==FVM_OK && std::fabs(counted_metric(c,field,"_width")-53.8)<0.02);
 fvm_value_boolean(&value,1);assert(fvm_object_set(c,format,"underline",&value)==FVM_OK && fvm_object_call(c,field,"setTextFormat",1,&arg,&out)==FVM_OK && std::fabs(counted_metric(c,field,"_width")-53.8)<0.02);
 FVM_Path *path=0;assert(fvm_textfield_path(c,field,&path)==FVM_OK);fvm_path_destroy(path);
 fvm_value_boolean(&value,0);assert(fvm_object_set(c,format,"kerning",&value)==FVM_OK);fvm_value_number(&value,20);assert(fvm_object_set(c,format,"size",&value)==FVM_OK);fvm_value_number(&value,-1);assert(fvm_object_set(c,format,"letterSpacing",&value)==FVM_OK && fvm_object_call(c,field,"setNewTextFormat",1,&arg,&out)==FVM_OK);
 assert(std::fabs(counted_metric(c,field,"_width")-53.8)<0.02);
 assert(fvm_value_string_bytes(&value,"A\0B",3)==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK && counted_metric(c,field,"_width")==37 && counted_metric(c,field,"_height")==26);
 assert(fvm_value_string(&value,"A\xcc\x81")==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_UNSUPPORTED);counted_text(c,field,"A\0B",3);
 assert(counted_metric(c,field,"_width")==37 && counted_metric(c,field,"_height")==26);
 assert(fvm_value_string(&value,"")==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK && counted_metric(c,field,"_width")==4 && counted_metric(c,field,"_height")==26);
 fvm_value_number(args,0);fvm_value_number(args+1,0);assert(fvm_value_string(args+2,"AV")==FVM_OK && fvm_object_call(c,field,"replaceText",3,args,&out)==FVM_OK && counted_metric(c,field,"_width")==26);
 assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);fvm_runtime_destroy(rt);
#endif
 return 0;
}
