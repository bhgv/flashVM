/* registerClass folding is shared by interpreted, explicit and automatic native calls. */
#define FVM_CLASS_CASE_FIXTURE_ONLY
#include "object_register_class_case.cpp"
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
int main()
{
 if(!fvm_jit_available())return 0;
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=0;version<=8;++version)for(class_mode=0;class_mode<3;++class_mode) {
  if(version && version<4)continue;FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);case_loaded(c,version);
  assert(fvm_standard_install(c)==FVM_OK);FVM_Object *one=case_ctor(c,11),*two=case_ctor(c,22),*found=0;FVM_Value value=case_object(one);
  assert(fvm_set_global(c,"SelectedClass",&value)==FVM_OK);
  Program p;p.variable("SelectedClass");p.text("Hero");p.number(2);p.variable("Object");p.text("registerClass");p.op(0x52);p.finish();
  value=class_run(c,p);assert(value.type==FVM_VALUE_BOOLEAN && value.boolean);
  assert(fvm_registered_class(c,"hero",&found)==(version && version<7?FVM_OK:FVM_NOT_FOUND));
  value=case_object(two);assert(fvm_set_global(c,"SelectedClass",&value)==FVM_OK);
  Program replace;replace.variable("SelectedClass");replace.text("hERO");replace.number(2);replace.variable("Object");replace.text("registerClass");replace.op(0x52);replace.finish();
  value=class_run(c,replace);assert(value.type==FVM_VALUE_BOOLEAN && value.boolean);
  assert(fvm_registered_class(c,"Hero",&found)==FVM_OK && found==(version && version<7?two:one));
  Program remove;remove.op(0x96);remove.op(1);remove.op(0);remove.op(2);remove.text("Hero");remove.number(2);remove.variable("Object");remove.text("registerClass");remove.op(0x52);remove.finish();
  value=class_run(c,remove);assert(value.type==FVM_VALUE_BOOLEAN && value.boolean);
  assert(fvm_registered_class(c,"hERO",&found)==(version && version<7?FVM_NOT_FOUND:FVM_OK));
  fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);
}
