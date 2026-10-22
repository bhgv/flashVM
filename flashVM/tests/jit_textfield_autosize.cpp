/* AVM1 property writes fit field geometry equally in interpreter and two native JIT modes. */
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include "textfield_counted_text.cpp"
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
int main()
{
 if(!fvm_jit_available())return 0;
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(class_mode=0;class_mode<3;++class_mode){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);
  const unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
  FVM_Object *field=counted_field(c),*format=0;FVM_Value value,out,arg;
  assert(fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK);fvm_value_number(&value,40);assert(fvm_object_set(c,format,"size",&value)==FVM_OK);
  arg=class_value(format);assert(fvm_object_call(c,field,"setNewTextFormat",1,&arg,&out)==FVM_OK);
  value=class_value(field);assert(fvm_set_global(c,"field",&value)==FVM_OK);
  assert(fvm_value_string(&value,"AV")==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK);
  for(unsigned step=0;step<3;++step){Program p;
   p.variable("field");p.text("autoSize");
   if(step==0)p.text("left");else{p.op(0x96);p.op(2);p.op(0);p.op(5);p.op(step==1?1:0);}p.op(0x4f);
   if(step){p.variable("field");p.text("text");p.text(step==1?"AVA":"A");p.op(0x4f);}
   p.variable("field");p.text("_width");p.op(0x4e);p.finish();out=class_run(c,p);
   assert(out.type==FVM_VALUE_NUMBER && out.number==(step==0?52:76));
   assert(counted_metric(c,field,"_height")==48 && fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth);
  }
  Program bad;bad.variable("field");bad.text("autoSize");bad.text("invalid");bad.op(0x4f);bad.number(1);bad.finish();class_run(c,bad,FVM_UNSUPPORTED);
  assert(counted_metric(c,field,"_width")==76);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);
#endif
 return 0;
}
