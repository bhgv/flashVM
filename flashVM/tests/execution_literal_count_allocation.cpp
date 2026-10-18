/* Count-conversion allocation failures preserve an aliased string input and root/depth state. */
#define main allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
 bool success=false;unsigned failures=0;
 for(long boundary=0;boundary<2000 && !success;++boundary)
 {
  FVM_Value v;fvm_value_string(&v,"1.9");FVM_GCRootFrame *roots=c->gc_frames;allowance=boundary;FVM_Result r=fvm_exec_literal_count(c,&v,&v);allowance=-1;
  assert(c->gc_frames==roots && !c->gc_native_depth && !c->script_depth);
  if(r==FVM_OK){assert(v.type==FVM_VALUE_NUMBER && v.number==1);success=true;}else{assert(r==FVM_OUT_OF_MEMORY && v.type==FVM_VALUE_STRING && v.string[0]=='1');++failures;}
  assert(fvm_gc_collect(c,0)==FVM_OK);
 }
 assert(success && failures);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
