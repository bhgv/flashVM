/* Registry replacement failures preserve both independent constructor environments. */
#define FVM_CLASS_CASE_ALLOCATION_FIXTURE_ONLY
#include "object_register_class_case_allocation.cpp"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=6;version<=8;version+=2){bool succeeded=false;unsigned failures=0;
  for(long boundary=0;boundary<4096 && !succeeded;++boundary){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);case_loaded(c,6);assert(fvm_standard_install(c)==FVM_OK);
   FVM_Value klass,args[2],out;assert(fvm_get_global(c,"Object",&klass)==FVM_OK);FVM_Object *one=case_ctor(c,11),*two=case_ctor(c,22),*three=case_ctor(c,33),*found=0;
   case_register(c,klass.object,"Hero",4,one);case_loaded(c,8);case_register(c,klass.object,"Hero",4,two);case_loaded(c,version);
   fvm_value_string(args,version==6?"hERO":"Hero");args[1]=case_object(three);allowance=boundary;FVM_Result r=fvm_object_call(c,klass.object,"registerClass",2,args,&out);allowance=-1;
   assert(r==FVM_OK || r==FVM_OUT_OF_MEMORY);succeeded=r==FVM_OK;if(!succeeded)++failures;
   assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth);
   assert(fvm_registered_class(c,"Hero",&found)==FVM_OK && found==(succeeded?three:version==6?one:two));
   case_loaded(c,version==6?8:6);assert(fvm_registered_class(c,"Hero",&found)==FVM_OK && found==(version==6?two:one));
   case_loaded(c,version);if(!succeeded)case_register(c,klass.object,"Hero",4,three);
   assert(fvm_registered_class(c,"Hero",&found)==FVM_OK && found==three);fvm_context_destroy(c);
  }
  assert(succeeded && failures);
 }
 fvm_runtime_destroy(rt);
}
