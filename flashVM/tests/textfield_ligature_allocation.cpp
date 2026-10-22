/* Admission of tracked styles and edits is atomic at every C++ allocation boundary. */
#define main allocation_reference_main
#include "textfield_counted_text_allocation.cpp"
#undef main
#include <cmath>
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned method=0;method<3;++method){bool success=false;unsigned failures=0;
  for(long boundary=0;boundary<4096 && !success;++boundary){
   FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);FVM_Object *field=counted_field(c),*format=0;
   FVM_Value value,arg,out,args[3];assert(fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK && fvm_gc_pin(c,format)==FVM_OK);
   fvm_value_number(&value,40);assert(fvm_object_set(c,format,"size",&value)==FVM_OK);
   fvm_value_number(&value,2);assert(fvm_object_set(c,format,"letterSpacing",&value)==FVM_OK);
   fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=format;
   assert(fvm_value_string(&value,"fi")==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK && fvm_object_call(c,field,"setTextFormat",1,&arg,&out)==FVM_OK && fvm_object_call(c,field,"setNewTextFormat",1,&arg,&out)==FVM_OK);
   double before=counted_metric(c,field,"textWidth");
   fvm_value_number(&value,3);assert(fvm_object_set(c,format,"letterSpacing",&value)==FVM_OK);
   assert(fvm_value_string(&value,"fifi")==FVM_OK);
   fvm_value_number(args,0);fvm_value_number(args+1,2);assert(fvm_value_string(args+2,"fifi")==FVM_OK);
   allowance=boundary;
   FVM_Result result=method==0?fvm_object_call(c,field,"setTextFormat",1,&arg,&out):method==1?fvm_object_set(c,field,"text",&value):fvm_object_call(c,field,"replaceText",3,args,&out);
   allowance=-1;assert(result==FVM_OK || result==FVM_OUT_OF_MEMORY);success=result==FVM_OK;if(!success)++failures;
   assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth);
   assert(std::fabs(counted_metric(c,field,"textWidth")-(success?(method==0?30:56):before))<0.02);
   counted_text(c,field,success?(method==1?"fifi":method==2?"fifi":"fi"):"fi",success&&method!=0?4:2);
   assert(fvm_object_call(c,field,"getTextFormat",0,0,&out)==FVM_OK && fvm_object_get(c,out.object,"letterSpacing",&value)==FVM_OK && value.type==FVM_VALUE_NUMBER && value.number==(success&&method==0?3:2));
   fvm_context_destroy(c);
  }
  assert(success && failures);
 }
 fvm_runtime_destroy(rt);
#endif
 return 0;
}
