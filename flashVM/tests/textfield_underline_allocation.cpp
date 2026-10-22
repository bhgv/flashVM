/* Every fallible style admission and decorated export preserves field/root state. */
#define main allocation_reference_main
#include "textfield_counted_text_allocation.cpp"
#undef main
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned method=0;method<3;++method){bool success=false;unsigned failures=0;
  for(long boundary=0;boundary<4096 && !success;++boundary){
   FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);FVM_Object *field=counted_field(c),*format=0;
   FVM_Value value,arg,out;assert(fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK && fvm_gc_pin(c,format)==FVM_OK);
   fvm_value_number(&value,40);assert(fvm_object_set(c,format,"size",&value)==FVM_OK);
   fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=format;
   assert(fvm_value_string(&value,"AV")==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK && fvm_object_call(c,field,"setTextFormat",1,&arg,&out)==FVM_OK);
   fvm_value_boolean(&value,1);assert(fvm_object_set(c,format,"underline",&value)==FVM_OK);
   if(method==2)assert(fvm_object_call(c,field,"setTextFormat",1,&arg,&out)==FVM_OK);
   assert(counted_metric(c,field,"textWidth")==48);
   FVM_Path *path=(FVM_Path*)1;allowance=boundary;
   FVM_Result result=method==2?fvm_textfield_path(c,field,&path):fvm_object_call(c,field,method==0?"setTextFormat":"setNewTextFormat",1,&arg,&out);
   allowance=-1;assert(result==FVM_OK || result==FVM_OUT_OF_MEMORY);success=result==FVM_OK;if(!success)++failures;
   if(method==2){if(success)fvm_path_destroy(path);else assert(!path);}
   assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth && counted_metric(c,field,"textWidth")==48);
   assert(fvm_object_call(c,field,method==1?"getNewTextFormat":"getTextFormat",0,0,&out)==FVM_OK && fvm_object_get(c,out.object,"underline",&value)==FVM_OK && value.type==FVM_VALUE_BOOLEAN && !!value.boolean==(method==2 || success));
   assert(fvm_textfield_path(c,field,&path)==FVM_OK);fvm_path_destroy(path);fvm_context_destroy(c);
  }
  assert(success && failures);
 }
 fvm_runtime_destroy(rt);
#endif
 return 0;
}
