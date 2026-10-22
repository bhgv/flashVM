/* Packed table bounds, integer/order rules, clearing and unsupported defaults are transactional. */
#include "textfield_tabs_fixture.hpp"
#include "../src/standard/text_state.hpp"
#include "../src/standard/text_constants.h"
#include <limits>
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);FVM_Object *field=counted_field(c);double stops[]={40,80};FVM_Object *format=tabs_format(c,stops,2);inset_apply(c,field,format,"setNewTextFormat");inset_string(c,field,"text","A\tV");FVM_TextFieldState good;assert(fvm_textfield_snapshot(field,good)==FVM_OK);
 for(unsigned kind=0;kind<9;++kind){FVM_TextFieldState bad=good;FVM_Context *d=0;FVM_Object *o=0;assert(fvm_context_create(rt,&d)==FVM_OK && fvm_standard_text_install(d)==FVM_OK && fvm_object_create(d,&o)==FVM_OK);o->resource_kind=FVM_RESOURCE_TEXT_FIELD;
  if(kind==0)bad.current.tab_stops.resize(65);
  if(kind==1)bad.current.tab_stops[0]=-1;
  if(kind==2)bad.current.tab_stops[0]=0.5;
  if(kind==3)bad.current.tab_stops[0]=80;
  if(kind==4)bad.current.tab_stops[0]=std::numeric_limits<double>::quiet_NaN();
  if(kind==5)bad.current.tab_stops[0]=1000001;
  if(kind==6)bad.current.tab_stops.clear();
  if(kind==7)bad.current.alignment&=~FVM_TEXT_STYLE_HAS_TABS;
  if(kind==8)bad.current.alignment|=4096UL;
  assert(fvm_textfield_restore(d,o,bad)==FVM_INVALID_ARGUMENT && !o->resource && d->fonts.empty());fvm_context_destroy(d);
 }
 FVM_Value check_arg,check_out,check_value;fvm_value_undefined(&check_arg);check_arg.type=FVM_VALUE_OBJECT;check_arg.object=format;
 const double invalid[]={-1,0.5,80,std::numeric_limits<double>::quiet_NaN()};for(unsigned i=0;i<6;++i){double entries[]={i<4?invalid[i]:40,80};FVM_Object *array=tabs_array(c,entries,i==4?0:2);if(i==4){fvm_value_number(&check_value,80);assert(fvm_table_set_index(c,array,1,&check_value)==FVM_OK);}if(i==5)inset_number(c,array,"length",65);fvm_value_undefined(&check_value);check_value.type=FVM_VALUE_OBJECT;check_value.object=array;assert(fvm_object_set(c,format,"tabStops",&check_value)==FVM_OK && fvm_object_call(c,field,"setTextFormat",1,&check_arg,&check_out)==FVM_INVALID_ARGUMENT && counted_metric(c,field,"textWidth")==64);}
 fvm_value_number(&check_value,0);assert(fvm_object_set(c,format,"tabStops",&check_value)==FVM_OK && fvm_object_call(c,field,"setTextFormat",1,&check_arg,&check_out)==FVM_UNSUPPORTED);check_value.type=FVM_VALUE_OBJECT;check_value.object=tabs_array(c,stops,2);assert(fvm_object_set(c,format,"tabStops",&check_value)==FVM_OK);
 inset_bool(c,field,"multiline",true);inset_bool(c,field,"wordWrap",true);
 FVM_Value arg,out,v;fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=format;inset_string(c,format,"align","justify");assert(fvm_object_call(c,field,"setTextFormat",1,&arg,&out)==FVM_UNSUPPORTED);inset_string(c,format,"align","left");FVM_Object *empty=tabs_array(c,0,0);v=arg;v.object=empty;assert(fvm_object_set(c,format,"tabStops",&v)==FVM_OK && fvm_object_call(c,field,"setTextFormat",1,&arg,&out)==FVM_UNSUPPORTED);counted_text(c,field,"A\tV",3);assert(counted_metric(c,field,"textWidth")==64);inset_string(c,field,"text","AV");inset_apply(c,field,format,"setTextFormat");inset_apply(c,field,format,"setNewTextFormat");assert(fvm_value_string(&v,"A\tV")==FVM_OK && fvm_object_set(c,field,"text",&v)==FVM_UNSUPPORTED);counted_text(c,field,"AV",2);
 fvm_context_destroy(c);fvm_runtime_destroy(rt);
#endif
 return 0;
}
