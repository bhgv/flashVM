/* Indexed dimension writes agree through interpreted and native SetProperty. */
#define FVM_DIMENSION_WRITE_FIXTURE_ONLY
#include "movieclip_dimension_write.cpp"
#define FVM_DISPLAY_FIXTURE_ONLY
#include "jit_display.cpp"
int main()
{
 if(!fvm_jit_available())return 0;FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=4;version<=8;++version)for(class_mode=0;class_mode<3;++class_mode){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);dimension_load(c,version);FVM_Object *clip=0,*root=display_setup(c,&clip);size_rect(c,clip,0,0,10,20);
  for(unsigned index=8;index<=9;++index){Program p;display_target(p);p.number(index);p.number(40);p.op(0x23);display_target(p);p.number(index);p.op(0x22);p.finish();FVM_Value out=class_run(c,p,FVM_OK,root);assert(out.type==FVM_VALUE_NUMBER && std::fabs(out.number-40)<1e-7);}
  assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);
}
