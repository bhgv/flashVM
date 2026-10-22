/* Uniform underline toggles geometry without changing shaping metrics; future style is independent. */
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include "textfield_counted_text.cpp"
#include <cmath>
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
 FVM_Object *field=counted_field(c),*format=0;FVM_Value value,arg,out;
 assert(fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK && fvm_gc_pin(c,format)==FVM_OK);
 assert(fvm_object_get(c,format,"underline",&value)==FVM_OK && value.type==FVM_VALUE_NULL);
 fvm_value_number(&value,40);assert(fvm_object_set(c,format,"size",&value)==FVM_OK);
 fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=format;
 assert(fvm_value_string(&value,"AV")==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK && fvm_object_call(c,field,"setTextFormat",1,&arg,&out)==FVM_OK);
 FVM_Path *base=0,*decorated=0;assert(fvm_textfield_path(c,field,&base)==FVM_OK);
 size_t commands=fvm_path_count(base);FVM_PathCommand point;
 fvm_value_boolean(&value,1);assert(fvm_object_set(c,format,"underline",&value)==FVM_OK && fvm_object_call(c,field,"setTextFormat",1,&arg,&out)==FVM_OK);
 assert(counted_metric(c,field,"textWidth")==48 && counted_metric(c,field,"textHeight")==44);
 assert(fvm_object_call(c,field,"getTextFormat",0,0,&out)==FVM_OK && fvm_object_get(c,out.object,"underline",&value)==FVM_OK && value.type==FVM_VALUE_BOOLEAN && value.boolean);
 assert(fvm_textfield_path(c,field,&decorated)==FVM_OK && fvm_path_count(decorated)==commands+5);
 assert(fvm_path_get(decorated,commands,&point)==FVM_OK && point.verb==FVM_PATH_MOVE && std::fabs(point.x[0]-2)<0.02 && std::fabs(point.y[0]-41)<0.02);
 assert(fvm_path_get(decorated,commands+2,&point)==FVM_OK && std::fabs(point.x[0]-50)<0.02 && std::fabs(point.y[0]-43)<0.02);
 fvm_path_destroy(decorated);
 fvm_value_boolean(&value,0);assert(fvm_object_set(c,format,"underline",&value)==FVM_OK && fvm_object_call(c,field,"setTextFormat",1,&arg,&out)==FVM_OK);
 assert(fvm_textfield_path(c,field,&decorated)==FVM_OK && fvm_path_count(decorated)==commands);fvm_path_destroy(decorated);
 fvm_value_boolean(&value,1);assert(fvm_object_set(c,format,"underline",&value)==FVM_OK && fvm_object_call(c,field,"setNewTextFormat",1,&arg,&out)==FVM_OK);
 assert(fvm_textfield_path(c,field,&decorated)==FVM_OK && fvm_path_count(decorated)==commands);fvm_path_destroy(decorated);
 assert(fvm_value_string(&value,"AV")==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK);
 assert(fvm_gc_collect(c,0)==FVM_OK && fvm_textfield_path(c,field,&decorated)==FVM_OK && fvm_path_count(decorated)==commands+5);
 fvm_path_destroy(base);fvm_context_destroy(c);fvm_runtime_destroy(rt);
 FVM_PathBounds bounds;assert(fvm_path_bounds(decorated,&bounds)==FVM_OK && std::fabs(bounds.y_max-43)<0.02);fvm_path_destroy(decorated);
#endif
 return 0;
}
