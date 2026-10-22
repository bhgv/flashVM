/* AVM1 inset writes change actual wrapping equally in interpreter and native JIT modes. */
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include "textfield_counted_text.cpp"
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#include <cmath>
int main()
{
 if(!fvm_jit_available())return 0;
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(class_mode=0;class_mode<3;++class_mode){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);
  const unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
  assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
  FVM_Object *field=counted_field(c),*format=0;FVM_Value value,out;
  assert(fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK);
  value=class_value(field);assert(fvm_set_global(c,"field",&value)==FVM_OK);
  value=class_value(format);assert(fvm_set_global(c,"format",&value)==FVM_OK);
  fvm_value_number(&value,40);assert(fvm_object_set(c,format,"size",&value)==FVM_OK);
  fvm_value_boolean(&value,1);assert(fvm_object_set(c,field,"multiline",&value)==FVM_OK && fvm_object_set(c,field,"wordWrap",&value)==FVM_OK);fvm_value_number(&value,124);assert(fvm_object_set(c,field,"_width",&value)==FVM_OK);
  assert(fvm_value_string(&value,"A V A V")==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK && fvm_value_string(&value,"left")==FVM_OK && fvm_object_set(c,field,"autoSize",&value)==FVM_OK);
  for(unsigned enabled=0;enabled<2;++enabled){Program p;
   p.variable("format");p.text("leftMargin");p.number(enabled?60:30);p.op(0x4f);
   p.variable("format");p.number(1);p.variable("field");p.text("setTextFormat");p.op(0x52);p.op(0x17);
   p.variable("field");p.text("textWidth");p.op(0x4e);p.finish();out=class_run(c,p);
   assert(out.type==FVM_VALUE_NUMBER && std::fabs(out.number-(enabled?60:72))<0.02);
   assert(counted_metric(c,field,"_height")== (enabled?140:94));
   assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth);
  }
  fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);
#endif
 return 0;
}
