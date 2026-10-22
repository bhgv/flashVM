/* Signed current/future formats, cache invalidation, unsupported clusters and GC. */
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include "textfield_counted_text.cpp"
#include <cmath>
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
 FVM_Object *field=counted_field(c),*format=0;FVM_Value arg,out,value;
 assert(fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK && fvm_gc_pin(c,format)==FVM_OK);
 assert(fvm_object_get(c,format,"letterSpacing",&value)==FVM_OK && value.type==FVM_VALUE_NULL);
 fvm_value_number(&value,40);assert(fvm_object_set(c,format,"size",&value)==FVM_OK);
 fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=format;
 assert(fvm_value_string(&value,"AV")==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK);
 assert(fvm_object_call(c,field,"setTextFormat",1,&arg,&out)==FVM_OK && std::fabs(counted_metric(c,field,"textWidth")-48)<0.02);
 for(unsigned step=0;step<3;++step){
  double spacing=step==0?2.5:step==1?-2:0;
  fvm_value_number(&value,spacing);assert(fvm_object_set(c,format,"letterSpacing",&value)==FVM_OK);
  assert(fvm_object_call(c,field,"setTextFormat",1,&arg,&out)==FVM_OK);
  assert(std::fabs(counted_metric(c,field,"textWidth")-(48+2*spacing))<0.02);
  assert(fvm_object_call(c,field,"getTextFormat",0,0,&out)==FVM_OK && fvm_object_get(c,out.object,"letterSpacing",&value)==FVM_OK && value.type==FVM_VALUE_NUMBER && value.number==spacing);
 }
 fvm_value_number(&value,3);assert(fvm_object_set(c,format,"letterSpacing",&value)==FVM_OK && fvm_object_call(c,field,"setNewTextFormat",1,&arg,&out)==FVM_OK);
 assert(std::fabs(counted_metric(c,field,"textWidth")-48)<0.02);
 assert(fvm_value_string(&value,"AVA")==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK && std::fabs(counted_metric(c,field,"textWidth")-81)<0.02);
 /* Failed edits and failed styles retain text, current style and cached geometry. */
 assert(fvm_value_string(&value,"A\xcc\x81")==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_UNSUPPORTED);
 assert(std::fabs(counted_metric(c,field,"textWidth")-81)<0.02);
 fvm_value_number(&value,-25);assert(fvm_object_set(c,format,"letterSpacing",&value)==FVM_OK && fvm_object_call(c,field,"setTextFormat",1,&arg,&out)==FVM_UNSUPPORTED);
 fvm_value_number(&value,4097);assert(fvm_object_set(c,format,"letterSpacing",&value)==FVM_OK && fvm_object_call(c,field,"setTextFormat",1,&arg,&out)==FVM_INVALID_ARGUMENT);
 assert(fvm_gc_collect(c,0)==FVM_OK && std::fabs(counted_metric(c,field,"textWidth")-81)<0.02);
 FVM_Path *path=0;assert(fvm_textfield_path(c,field,&path)==FVM_OK && path);fvm_path_destroy(path);
 fvm_context_destroy(c);fvm_runtime_destroy(rt);
#endif
 return 0;
}
