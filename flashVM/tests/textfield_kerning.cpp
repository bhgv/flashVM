/* AS2 disabled default, explicit enable/disable, cache invalidation, future format and path export. */
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
 assert(fvm_object_get(c,format,"kerning",&value)==FVM_OK && value.type==FVM_VALUE_BOOLEAN && !value.boolean);
 fvm_value_number(&value,40);assert(fvm_object_set(c,format,"size",&value)==FVM_OK);
 fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=format;
 assert(fvm_value_string(&value,"AV")==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK);
 assert(fvm_object_call(c,field,"setTextFormat",1,&arg,&out)==FVM_OK && std::fabs(counted_metric(c,field,"textWidth")-48)<0.02);
 for(unsigned enabled=1;enabled<=2;++enabled){
  fvm_value_boolean(&value,enabled==1);assert(fvm_object_set(c,format,"kerning",&value)==FVM_OK);
  assert(fvm_object_call(c,field,"setTextFormat",1,&arg,&out)==FVM_OK);
  assert(std::fabs(counted_metric(c,field,"textWidth")-(enabled==1?44.8:48))<0.02);
  assert(fvm_object_call(c,field,"getTextFormat",0,0,&out)==FVM_OK && fvm_object_get(c,out.object,"kerning",&value)==FVM_OK && value.type==FVM_VALUE_BOOLEAN && value.boolean==(enabled==1));
 }
 fvm_value_boolean(&value,1);assert(fvm_object_set(c,format,"kerning",&value)==FVM_OK && fvm_object_call(c,field,"setNewTextFormat",1,&arg,&out)==FVM_OK);
 assert(std::fabs(counted_metric(c,field,"textWidth")-48)<0.02);
 assert(fvm_value_string(&value,"AVA")==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK && std::fabs(counted_metric(c,field,"textWidth")-68.8)<0.02);
 FVM_Path *path=0;assert(fvm_textfield_path(c,field,&path)==FVM_OK && path);fvm_path_destroy(path);
 assert(fvm_gc_collect(c,0)==FVM_OK && std::fabs(counted_metric(c,field,"textWidth")-68.8)<0.02);
 fvm_context_destroy(c);fvm_runtime_destroy(rt);
#endif
 return 0;
}
