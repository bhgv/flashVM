/* Getter side effects preserve native collector barriers and rollback; collection follows the call. */
#include "textfield_tabs_fixture.hpp"
struct TabProbe {FVM_Object *format;unsigned calls,mode;};
static FVM_Result tab_getter(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *raw)
{TabProbe *p=(TabProbe*)raw;++p->calls;FVM_Value v;fvm_value_undefined(&v);v.type=FVM_VALUE_NULL;assert(fvm_object_set(c,p->format,"tabStops",&v)==FVM_OK && fvm_gc_collect(c,0)==FVM_INVALID_ARGUMENT);fvm_value_number(out,p->mode==2?120:60);return p->mode==1?FVM_ERROR:FVM_OK;}
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);FVM_Object *field=counted_field(c);double stops[]={40,80};FVM_Object *format=tabs_format(c,stops,2);inset_apply(c,field,format,"setNewTextFormat");inset_string(c,field,"text","A\tV");inset_string(c,field,"autoSize","left");
 for(unsigned mode=0;mode<3;++mode){FVM_Object *array=tabs_array(c,0,0);FVM_Value v,out,arg;fvm_value_number(&v,120);assert(fvm_table_set_index(c,array,1,&v)==FVM_OK);TabProbe probe={format,0,mode};assert(fvm_object_define_accessor_rooted(c,array,"0",tab_getter,0,&probe,0)==FVM_OK);fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=array;assert(fvm_object_set(c,format,"tabStops",&v)==FVM_OK && fvm_gc_unpin(c,array)==FVM_OK);arg=v;arg.object=format;assert(fvm_object_call(c,field,"setTextFormat",1,&arg,&out)==(mode==0?FVM_OK:mode==1?FVM_ERROR:FVM_INVALID_ARGUMENT));assert(probe.calls==1 && !c->gc_frames && !c->gc_native_depth && counted_metric(c,field,"textWidth")==84 && counted_metric(c,field,"_width")==88 && fvm_gc_collect(c,0)==FVM_OK);counted_text(c,field,"A\tV",3);assert(fvm_object_get(c,format,"tabStops",&v)==FVM_OK && v.type==FVM_VALUE_NULL);}
 fvm_context_destroy(c);fvm_runtime_destroy(rt);
#endif
 return 0;
}
