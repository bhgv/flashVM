/* Applied tab tables are copied; source/snapshot Array mutation cannot edit the native field. */
#include "textfield_tabs_fixture.hpp"
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);FVM_Object *field=counted_field(c);double stops[]={40,80};FVM_Object *format=tabs_format(c,stops,2);inset_apply(c,field,format,"setNewTextFormat");inset_string(c,field,"text","A\tV\tA");inset_string(c,field,"autoSize","left");assert(counted_metric(c,field,"textWidth")==104 && counted_metric(c,field,"_width")==108 && counted_metric(c,field,"_height")==48);
 FVM_Value a,v,out;assert(fvm_object_get(c,format,"tabStops",&a)==FVM_OK);fvm_value_number(&v,60);assert(fvm_table_set_index(c,a.object,0,&v)==FVM_OK && counted_metric(c,field,"textWidth")==104);inset_string(c,field,"text","A\tV\tA");assert(counted_metric(c,field,"textWidth")==104);assert(fvm_object_call(c,field,"getTextFormat",0,0,&out)==FVM_OK && fvm_object_get(c,out.object,"tabStops",&a)==FVM_OK && counted_metric(c,a.object,"length")==2);assert(fvm_table_set_index(c,a.object,0,&v)==FVM_OK && counted_metric(c,field,"textWidth")==104);assert(fvm_gc_collect(c,0)==FVM_OK);FVM_Path *p=0;assert(fvm_textfield_path(c,field,&p)==FVM_OK);fvm_context_destroy(c);assert(fvm_path_count(p));fvm_path_destroy(p);fvm_runtime_destroy(rt);
#endif
 return 0;
}
