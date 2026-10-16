/* Base-movie registration survives native calls under a different execution version. */
#define FVM_CLASS_PROVENANCE_FIXTURE_ONLY
#include "object_register_class_provenance.cpp"
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
int main()
{
 if(!fvm_jit_available())return 0;FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned source=4;source<=8;++source)for(unsigned ambient=6;ambient<=8;ambient+=2)for(class_mode=0;class_mode<3;++class_mode){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);case_loaded(c,ambient);assert(fvm_standard_install(c)==FVM_OK);
  FVM_Object *base=provenance_root(c,source),*fn=case_ctor(c,11),*found=0;FVM_Value value=case_object(fn);assert(fvm_set_global(c,"registrationCtor",&value)==FVM_OK);
  Program p;p.variable("registrationCtor");p.text("Hero");p.number(2);p.variable("Object");p.text("registerClass");p.op(0x52);p.finish();
  value=class_run(c,p,FVM_OK,base);assert(value.type==FVM_VALUE_BOOLEAN && value.boolean && c->swf_info.version==ambient);
  bool same=(source<7)==(ambient<7);assert(fvm_registered_class(c,"Hero",&found)==(same?FVM_OK:FVM_NOT_FOUND));case_loaded(c,source);assert(fvm_registered_class(c,"Hero",&found)==FVM_OK && found==fn);
  assert(fvm_gc_collect(c,0)==FVM_OK && !c->avm1_target_active && !c->gc_frames && !c->gc_native_depth);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);
}
