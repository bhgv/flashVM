/* Every fallible current/future format acquisition leaves live kerning and cached geometry unchanged. */
#define main allocation_reference_main
#include "textfield_counted_text_allocation.cpp"
#undef main
#include <cmath>
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned future=0;future<2;++future){bool success=false;unsigned failures=0;
  for(long boundary=0;boundary<256 && !success;++boundary){
   FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);FVM_Object *field=counted_field(c),*format=0;
   FVM_Value value,arg,out;assert(fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK && fvm_gc_pin(c,format)==FVM_OK);
   fvm_value_number(&value,40);assert(fvm_object_set(c,format,"size",&value)==FVM_OK);
   fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=format;
   assert(fvm_value_string(&value,"AV")==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK && fvm_object_call(c,field,"setTextFormat",1,&arg,&out)==FVM_OK);
   double before=counted_metric(c,field,"textWidth");
   fvm_value_boolean(&value,1);assert(fvm_object_set(c,format,"kerning",&value)==FVM_OK);
   const char *method=future?"setNewTextFormat":"setTextFormat";
   allowance=boundary;FVM_Result result=fvm_object_call(c,field,method,1,&arg,&out);allowance=-1;
   assert(result==FVM_OK || result==FVM_OUT_OF_MEMORY);success=result==FVM_OK;if(!success)++failures;
   assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth);
   assert(std::fabs(counted_metric(c,field,"textWidth")-((success && !future)?44.8:before))<0.02);
   assert(fvm_object_call(c,field,future?"getNewTextFormat":"getTextFormat",0,0,&out)==FVM_OK && fvm_object_get(c,out.object,"kerning",&value)==FVM_OK && value.type==FVM_VALUE_BOOLEAN && !!value.boolean==success);
   if(!success)assert(fvm_object_call(c,field,method,1,&arg,&out)==FVM_OK);
   if(future)assert(fvm_value_string(&value,"AVA")==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK && std::fabs(counted_metric(c,field,"textWidth")-68.8)<0.02);
   fvm_context_destroy(c);
  }
  assert(success && failures);
 }
 fvm_runtime_destroy(rt);
#endif
 return 0;
}
