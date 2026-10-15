/* Large cyclic interface membership shares native/interpreter work accounting. */
#define FVM_INSTANCE_GRAPH_FIXTURE_ONLY
#include "instance_of_large_graph.cpp"
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
int main()
{
 if(!fvm_jit_available())return 0;FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=4;version<=8;++version)for(class_mode=0;class_mode<3;++class_mode) {
  FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);graph_load(c,version);
  FVM_Value klass,input=graph_setup(c,4097,&klass,true),out;
  assert(fvm_set_global(c,"instance",&input)==FVM_OK && fvm_set_global(c,"InterfaceClass",&klass)==FVM_OK);
  Program p;p.variable("instance");p.variable("InterfaceClass");p.op(0x54);p.finish();
  out=class_run(c,p);assert(out.type==FVM_VALUE_BOOLEAN && out.boolean);
  unsigned long limit=rt->config.instruction_limit;rt->config.instruction_limit=32;
  class_run(c,p,FVM_ERROR);assert(c->avm1_steps==32 && !c->avm1_execution_depth);
  rt->config.instruction_limit=limit;out=class_run(c,p);assert(out.boolean);
  assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);
}
