/* Native SetMember preserves same counted text formatting and applies future format on change. */
#define FVM_SAME_FIELD_FIXTURE_ONLY
#include "textfield_same_text.cpp"
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
int main()
{
 if(!fvm_jit_available())return 0;FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
#ifdef FVM_HAS_FONTS
 for(unsigned version=4;version<=8;++version)for(class_mode=0;class_mode<3;++class_mode){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);same_load(c,version);assert(fvm_standard_install(c)==FVM_OK);FVM_Object *field=counted_field(c);FVM_Value value,out;
  assert(fvm_value_string_bytes(&value,"A\0B",3)==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK && fvm_set_global(c,"sameText",&value)==FVM_OK);same_future(c,field,20);value=class_value(field);assert(fvm_set_global(c,"field",&value)==FVM_OK);
  Program p;p.variable("field");p.text("text");p.variable("sameText");p.op(0x4f);p.variable("field");p.text("text");p.op(0x4e);p.finish();out=class_run(c,p);
  assert(out.type==FVM_VALUE_STRING && fvm_value_string_size(&out)==3 && !std::memcmp(out.string,"A\0B",3) && same_format(c,field,false)==12 && same_format(c,field,true)==20);
  assert(fvm_value_string_bytes(&value,"A\0C",3)==FVM_OK && fvm_set_global(c,"sameText",&value)==FVM_OK);out=class_run(c,p);assert(out.type==FVM_VALUE_STRING && fvm_value_string_size(&out)==3 && !std::memcmp(out.string,"A\0C",3) && same_format(c,field,false)==20);
  assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth);counted_text(c,field,"A\0C",3);fvm_context_destroy(c);
 }
#endif
 fvm_runtime_destroy(rt);
}
