/* Indexed vector dimensions agree in interpreter, explicit JIT and automatic JIT. */
#define FVM_DISPLAY_FIXTURE_ONLY
#include "jit_display.cpp"
#include <cmath>
static void point(FVM_Context *c,FVM_Object *o,const char *method,double x,double y)
{FVM_Value a[2],out;fvm_value_number(a,x);fvm_value_number(a+1,y);assert(fvm_object_call(c,o,method,2,a,&out)==FVM_OK);}
int main()
{
 if(!fvm_jit_available())return 0;
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=0;version<=8;++version)for(class_mode=0;class_mode<3;++class_mode) {
  if(version && version<4)continue;
  FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);
  unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
  if(version)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
  FVM_Object *clip=0,*root=display_setup(c,&clip);FVM_Value a,out;
  fvm_value_number(&a,0xff0000);assert(fvm_object_call(c,clip,"beginFill",1,&a,&out)==FVM_OK);
  point(c,clip,"moveTo",0,0);point(c,clip,"lineTo",10,0);point(c,clip,"lineTo",10,20);point(c,clip,"lineTo",0,20);point(c,clip,"lineTo",0,0);
  assert(fvm_object_call(c,clip,"endFill",0,0,&out)==FVM_OK);
  fvm_value_number(&a,0);assert(fvm_object_set(c,root,"_xscale",&a)==FVM_OK);
  fvm_value_number(&a,-200);assert(fvm_object_set(c,clip,"_xscale",&a)==FVM_OK);
  fvm_value_number(&a,300);assert(fvm_object_set(c,clip,"_yscale",&a)==FVM_OK);
  fvm_value_number(&a,90);assert(fvm_object_set(c,clip,"_rotation",&a)==FVM_OK);
  for(unsigned index=8;index<=9;++index) {
   Program p;display_target(p);p.number(index);p.op(0x22);p.finish();out=class_run(c,p,FVM_OK,root);
   assert(out.type==FVM_VALUE_NUMBER && std::fabs(out.number-(index==8?60:20))<1e-7);
  }
  Program missing;missing.text("absent");missing.number(8);missing.op(0x22);missing.finish();
  out=class_run(c,missing,FVM_OK,root);assert(out.type==FVM_VALUE_UNDEFINED);
  fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);
}
