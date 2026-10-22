/* Word wrapping preserves outer width while edits and width constraints fit height. */
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include "textfield_counted_text.cpp"
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
 FVM_Object *field=counted_field(c),*format=0;FVM_Value value,arg,out;
 assert(fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK);fvm_value_number(&value,40);assert(fvm_object_set(c,format,"size",&value)==FVM_OK);
 fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=format;assert(fvm_object_call(c,field,"setNewTextFormat",1,&arg,&out)==FVM_OK);
 fvm_value_number(&value,54);assert(fvm_object_set(c,field,"_width",&value)==FVM_OK);
 fvm_value_boolean(&value,1);assert(fvm_object_set(c,field,"multiline",&value)==FVM_OK && fvm_object_set(c,field,"wordWrap",&value)==FVM_OK);
 assert(fvm_value_string(&value,"AVAV")==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK);
 fvm_value_boolean(&value,1);assert(fvm_object_set(c,field,"autoSize",&value)==FVM_OK && counted_metric(c,field,"_width")==54 && counted_metric(c,field,"_height")==94);
 fvm_value_number(&value,30);assert(fvm_object_set(c,field,"_width",&value)==FVM_OK && counted_metric(c,field,"_width")==30 && counted_metric(c,field,"_height")==186);
 fvm_value_boolean(&value,0);assert(fvm_object_set(c,field,"wordWrap",&value)==FVM_OK && counted_metric(c,field,"_width")==100 && counted_metric(c,field,"_height")==48);
 fvm_value_number(&value,55);assert(fvm_object_set(c,field,"_width",&value)==FVM_UNSUPPORTED && fvm_object_set(c,field,"_height",&value)==FVM_UNSUPPORTED);
 fvm_value_boolean(&value,1);assert(fvm_object_set(c,field,"wordWrap",&value)==FVM_OK && counted_metric(c,field,"_width")==100 && counted_metric(c,field,"_height")==48);
 fvm_value_boolean(&value,0);assert(fvm_object_set(c,field,"multiline",&value)==FVM_OK);
 assert(fvm_value_string(&value,"AV\nA")==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_UNSUPPORTED && counted_metric(c,field,"_height")==48);
 fvm_value_boolean(&value,1);assert(fvm_object_set(c,field,"multiline",&value)==FVM_OK);
 assert(fvm_value_string(&value,"AV\nA")==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK && counted_metric(c,field,"_width")==100 && counted_metric(c,field,"_height")==94);
 fvm_value_boolean(&value,0);assert(fvm_object_set(c,field,"multiline",&value)==FVM_UNSUPPORTED && counted_metric(c,field,"_height")==94);
 assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);fvm_runtime_destroy(rt);
#endif
 return 0;
}
