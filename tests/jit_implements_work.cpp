/* Interface declaration budget failure is atomic in interpreted and native execution. */
#define FVM_IMPLEMENTS_WORK_FIXTURE_ONLY
#include "implements_work.cpp"
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
int main()
{
 if(!fvm_jit_available())return 0;FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=4;version<=8;++version)for(class_mode=0;class_mode<3;++class_mode){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);declaration_load(c,version);
  FVM_Object *subproto=0,*ifaceproto=0;FVM_Value sub=declaration_class(c,&subproto),iface=declaration_class(c,&ifaceproto);
  assert(fvm_set_global(c,"Sub",&sub)==FVM_OK && fvm_set_global(c,"Iface",&iface)==FVM_OK);
  Program p;for(unsigned i=0;i<4;++i)p.variable("Iface");p.number(4);p.variable("Sub");p.op(0x2c);p.number(42);p.finish();
  unsigned long limit=rt->config.instruction_limit;rt->config.instruction_limit=16;
  FVM_Value out=class_run(c,p,version>=7?FVM_ERROR:FVM_OK);assert(!subproto->interfaces_are_prototypes && subproto->implemented_interfaces.empty());
  rt->config.instruction_limit=limit;out=class_run(c,p);assert(out.type==FVM_VALUE_NUMBER && out.number==42 && subproto->interfaces_are_prototypes==(version>=7));
  if(version>=7){assert(subproto->implemented_interfaces.size()==4);for(unsigned i=0;i<4;++i)assert(subproto->implemented_interfaces[i]==ifaceproto);}
  assert(fvm_gc_collect(c,0)==FVM_OK);declaration_clean(c);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);
}
