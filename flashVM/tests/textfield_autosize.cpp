/* Left-anchored automatic fields resize on edits and current-format changes. */
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include "textfield_counted_text.cpp"
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
 FVM_Object *field=counted_field(c),*format=0;FVM_Value value,arg,out,args[3];
 assert(fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK && fvm_gc_pin(c,format)==FVM_OK);
 fvm_value_number(&value,40);assert(fvm_object_set(c,format,"size",&value)==FVM_OK);
 fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=format;
 assert(fvm_object_call(c,field,"setNewTextFormat",1,&arg,&out)==FVM_OK);
 assert(fvm_value_string(&value,"AV")==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK);
 assert(fvm_object_get(c,field,"autoSize",&value)==FVM_OK && value.type==FVM_VALUE_STRING && !std::strcmp(value.string,"none"));
 assert(fvm_value_string(&value,"left")==FVM_OK && fvm_object_set(c,field,"autoSize",&value)==FVM_OK);
 assert(counted_metric(c,field,"_width")==52 && counted_metric(c,field,"_height")==48);
 assert(fvm_value_string(&value,"A")==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK && counted_metric(c,field,"_width")==28);
 fvm_value_number(args,0);fvm_value_number(args+1,1);assert(fvm_value_string(args+2,"AVA")==FVM_OK && fvm_object_call(c,field,"replaceText",3,args,&out)==FVM_OK && counted_metric(c,field,"_width")==76);
 fvm_value_number(&value,20);assert(fvm_object_set(c,format,"size",&value)==FVM_OK && fvm_object_call(c,field,"setTextFormat",1,&arg,&out)==FVM_OK);
 assert(counted_metric(c,field,"_width")==40 && counted_metric(c,field,"_height")==26);
 fvm_value_boolean(&value,0);assert(fvm_object_set(c,field,"autoSize",&value)==FVM_OK);
 assert(fvm_value_string(&value,"AV")==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK && counted_metric(c,field,"_width")==40 && counted_metric(c,field,"_height")==26);
 assert(fvm_value_string(&value,"invalid-center")==FVM_OK && fvm_object_set(c,field,"autoSize",&value)==FVM_UNSUPPORTED);
 assert(fvm_value_string(&value,"invalid-right")==FVM_OK && fvm_object_set(c,field,"autoSize",&value)==FVM_UNSUPPORTED);
 fvm_value_boolean(&value,1);assert(fvm_object_set(c,field,"autoSize",&value)==FVM_OK && counted_metric(c,field,"_width")==52);
 FVM_Path *path=0;assert(fvm_gc_collect(c,0)==FVM_OK && fvm_textfield_path(c,field,&path)==FVM_OK);fvm_path_destroy(path);
 fvm_context_destroy(c);fvm_runtime_destroy(rt);
#endif
 return 0;
}
