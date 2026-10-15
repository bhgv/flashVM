/* Failed loaded count allocations preserve outputs; host count preparation needs no allocation. */
#define main allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};FVM_Value v;unsigned out=99;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
 fvm_value_number(&v,65536);allowance=0;assert(fvm_exec_constructor_count(c,&v,65536,&out)==FVM_OK && out==65536);allowance=-1;
 assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);bool success=false;unsigned failures=0;
 for(long boundary=0;boundary<2000 && !success;++boundary)
 {
  fvm_value_string(&v,"1.9");out=99;FVM_GCRootFrame *roots=c->gc_frames;allowance=boundary;FVM_Result r=fvm_exec_constructor_count(c,&v,7,&out);allowance=-1;
  assert(c->gc_frames==roots && !c->gc_native_depth && !c->script_depth);
  if(r==FVM_OK){assert(out==1);success=true;}else{assert(r==FVM_OUT_OF_MEMORY && out==99);++failures;}
  assert(fvm_gc_collect(c,0)==FVM_OK);
 }
 assert(success && failures);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
