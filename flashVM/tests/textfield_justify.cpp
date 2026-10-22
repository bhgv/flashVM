/* Native fields use justified widths and preserve the policy in format snapshots. */
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include "textfield_counted_text.cpp"
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
 FVM_Object *field=counted_field(c),*format=0;FVM_Value arg,v,out;assert(fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK);
 fvm_value_number(&v,40);assert(fvm_object_set(c,format,"size",&v)==FVM_OK);assert(fvm_value_string(&v,"justify")==FVM_OK && fvm_object_set(c,format,"align",&v)==FVM_OK);
 fvm_value_boolean(&v,1);assert(fvm_object_set(c,field,"multiline",&v)==FVM_OK && fvm_object_set(c,field,"wordWrap",&v)==FVM_OK);fvm_value_number(&v,94);assert(fvm_object_set(c,field,"_width",&v)==FVM_OK);
 fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=format;assert(fvm_object_call(c,field,"setNewTextFormat",1,&arg,&out)==FVM_OK && fvm_value_string(&v,"A V A V")==FVM_OK && fvm_object_set(c,field,"text",&v)==FVM_OK);
 assert(counted_metric(c,field,"textWidth")==90 && counted_metric(c,field,"textHeight")==90);
 assert(fvm_value_string(&v,"right")==FVM_OK && fvm_object_set(c,field,"autoSize",&v)==FVM_OK && counted_metric(c,field,"_height")==94);
 assert(fvm_object_call(c,field,"getTextFormat",0,0,&out)==FVM_OK && fvm_object_get(c,out.object,"align",&v)==FVM_OK && !std::strcmp(v.string,"justify"));
 assert(fvm_value_string(&v,"\xd7\x90 \xd7\x91")==FVM_OK && fvm_object_set(c,field,"text",&v)==FVM_UNSUPPORTED);counted_text(c,field,"A V A V",7);
 FVM_Path *path=0;assert(fvm_gc_collect(c,0)==FVM_OK && fvm_textfield_path(c,field,&path)==FVM_OK);fvm_context_destroy(c);assert(fvm_path_count(path)>0);fvm_path_destroy(path);fvm_runtime_destroy(rt);
#endif
 return 0;
}
