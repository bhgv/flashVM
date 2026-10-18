/* Every literal allocation failure preserves aliased outputs and restores temporary roots. */
#define main allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned mode=0;mode<2;++mode)
 {
  FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);bool success=false;unsigned failures=0;
  for(long boundary=0;boundary<2000 && !success;++boundary)
  {
   FVM_Value values[2];FVM_ExecutionLiteralEntry entries[2];fvm_value_number(values,17);fvm_value_number(values+1,19);
   fvm_value_string(&entries[0].name,"first");entries[0].value=values[0];fvm_value_string(&entries[1].name,"second");entries[1].value=values[1];
   FVM_Value *out=mode?&entries[0].value:values;FVM_GCRootFrame *roots=c->gc_frames;
   allowance=boundary;FVM_Result r=mode?fvm_exec_literal_object(c,2,entries,out):fvm_exec_literal_array(c,2,values,out);allowance=-1;
   assert(c->gc_frames==roots && !c->gc_native_depth && !c->script_depth);
   if(r==FVM_OK){assert(out->type==FVM_VALUE_OBJECT);FVM_Value v;assert(fvm_object_get(c,out->object,mode?"second":"1",&v)==FVM_OK && v.number==19);success=true;}
   else{assert(r==FVM_OUT_OF_MEMORY && out->type==FVM_VALUE_NUMBER && out->number==17);++failures;}
   assert(fvm_gc_collect(c,0)==FVM_OK);
  }
  assert(success && failures);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);return 0;
}
