/* Allocation failures preserve field geometry and font-derived intervals in current/future styles. */
#define main allocation_reference_main
#include "textfield_counted_text_allocation.cpp"
#undef main
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned method=0;method<3;++method){bool success=false;unsigned failures=0;for(long boundary=0;boundary<4096 && !success;++boundary){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);FVM_Object *field=counted_field(c),*format=0;FVM_Value v,arg,out,edit,args[3];assert(fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK && fvm_gc_pin(c,format)==FVM_OK);fvm_value_number(&v,40);assert(fvm_object_set(c,format,"size",&v)==FVM_OK);fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=format;assert(fvm_object_call(c,field,"setNewTextFormat",1,&arg,&out)==FVM_OK && fvm_value_string(&v,"A\tV")==FVM_OK && fvm_object_set(c,field,"text",&v)==FVM_OK);fvm_value_boolean(&v,1);assert(fvm_object_set(c,field,"autoSize",&v)==FVM_OK);
  fvm_value_number(&v,20);assert(fvm_object_set(c,format,"size",&v)==FVM_OK && fvm_value_string(&edit,"A\tV\tA")==FVM_OK);fvm_value_number(args,0);fvm_value_number(args+1,3);args[2]=edit;
  allowance=boundary;FVM_Result result=method==0?fvm_object_call(c,field,"setTextFormat",1,&arg,&out):method==1?fvm_object_set(c,field,"text",&edit):fvm_object_call(c,field,"replaceText",3,args,&out);allowance=-1;assert(result==FVM_OK || result==FVM_OUT_OF_MEMORY);success=result==FVM_OK;if(!success)++failures;
  assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth);double expected=success?(method?199:55.75):111.5;assert(counted_metric(c,field,"textWidth")==expected && counted_metric(c,field,"_width")==expected+4 && counted_metric(c,field,"_height")== (success&&!method?26:48));counted_text(c,field,success&&method?"A\tV\tA":"A\tV",success&&method?5:3);
  FVM_Path *path=0;assert(fvm_textfield_path(c,field,&path)==FVM_OK && fvm_path_count(path)>0);fvm_path_destroy(path);fvm_context_destroy(c);
 }assert(success && failures);}fvm_runtime_destroy(rt);
#endif
 return 0;
}
