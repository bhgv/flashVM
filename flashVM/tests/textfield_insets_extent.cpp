/* Extent, empty fitting, anchoring, null merge and explicit inset admission boundaries. */
#define FVM_INSETS_FIXTURE_ONLY
#include "textfield_insets_fixture.hpp"
#include <limits>
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
 FVM_Object *field=counted_field(c),*format=inset_format(c);inset_apply(c,field,format,"setNewTextFormat");inset_string(c,field,"text","A V");FVM_Value args[2],v,out,arg;assert(fvm_value_string(args,"A V A V")==FVM_OK);fvm_value_number(args+1,120);
 assert(fvm_object_call(c,format,"getTextExtent",2,args,&out)==FVM_OK && counted_metric(c,out.object,"width")==72 && counted_metric(c,out.object,"textFieldWidth")==106 && counted_metric(c,out.object,"height")==90);
 inset_number(c,field,"_x",100);inset_number(c,field,"_width",124);inset_string(c,field,"autoSize","center");assert(counted_metric(c,field,"_width")==94 && counted_metric(c,field,"_x")==115);
 inset_string(c,field,"text","");assert(counted_metric(c,field,"_width")==34 && counted_metric(c,field,"_x")==145 && counted_metric(c,field,"_height")==48);
 const char *names[]={"leftMargin","rightMargin","blockIndent"};const double bad[]={-1,0.5,4097,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()};
 fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=format;
 for(unsigned name=0;name<3;++name){double original=name==0?10:name==1?12:8;
  for(unsigned i=0;i<5;++i){inset_number(c,format,names[name],bad[i]);assert(fvm_object_call(c,field,"setTextFormat",1,&arg,&out)==FVM_INVALID_ARGUMENT && counted_metric(c,field,"_width")==34 && counted_metric(c,field,"_x")==145);}
  fvm_value_undefined(&v);v.type=FVM_VALUE_NULL;assert(fvm_object_set(c,format,names[name],&v)==FVM_OK);inset_apply(c,field,format,"setTextFormat");assert(fvm_object_call(c,field,"getTextFormat",0,0,&out)==FVM_OK && counted_metric(c,out.object,names[name])==original);inset_number(c,format,names[name],original);
 }
 inset_number(c,format,"leftMargin",4096);inset_number(c,format,"rightMargin",4096);inset_number(c,format,"blockIndent",4096);inset_apply(c,field,format,"setTextFormat");assert(counted_metric(c,field,"_width")==12292);inset_bool(c,field,"multiline",true);inset_bool(c,field,"wordWrap",true);inset_number(c,field,"_width",4);inset_apply(c,field,format,"setNewTextFormat");inset_string(c,field,"text","AV");assert(counted_metric(c,field,"_width")==4 && counted_metric(c,field,"textHeight")==90);
 fvm_context_destroy(c);fvm_runtime_destroy(rt);
#endif
 return 0;
}
