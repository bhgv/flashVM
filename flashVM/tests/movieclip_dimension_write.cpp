/* Indexed size writes update both scales from untransformed vector bounds. */
#define FVM_INDEXED_SIZE_FIXTURE_ONLY
#include "movieclip_indexed_size.cpp"
#include <limits>
#include <cfloat>
static void dimension_load(FVM_Context *c,unsigned version)
{if(version){unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);}assert(fvm_standard_install(c)==FVM_OK);}
static void dimension_scale(FVM_Context *c,FVM_Object *clip,double x,double y)
{FVM_Value out;assert(fvm_object_get(c,clip,"_xscale",&out)==FVM_OK && std::fabs(out.number-x)<1e-7);assert(fvm_object_get(c,clip,"_yscale",&out)==FVM_OK && std::fabs(out.number-y)<1e-7);}
static void dimension_write(FVM_Context *c,FVM_Object *clip,unsigned index,double requested)
{FVM_Value target=size_object(clip),at,value,out;fvm_value_number(&at,index);fvm_value_number(&value,requested);assert(fvm_exec_display_property(c,1,&target,&at,&value,&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);}
#ifndef FVM_DIMENSION_WRITE_FIXTURE_ONLY
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=0;version<=8;++version){if(version && version<4)continue;FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);dimension_load(c,version);
  FVM_Object *root=0;assert(fvm_movieclip_create(c,"root",1,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);FVM_Object *clip=child(c,root,"clip",1);
  dimension_write(c,clip,8,40);dimension_scale(c,clip,0,0);size_rect(c,clip,0,0,10,20);size_scalar(c,clip,"_xscale",-200);size_scalar(c,clip,"_yscale",300);
  dimension_write(c,clip,8,40);dimension_scale(c,clip,400,300);size_expect(c,clip,8,40);dimension_write(c,clip,9,-80);dimension_scale(c,clip,400,-400);size_expect(c,clip,9,80);
  size_scalar(c,clip,"_xscale",200);size_scalar(c,clip,"_yscale",300);size_scalar(c,clip,"_rotation",45);
  dimension_write(c,clip,8,40);dimension_scale(c,clip,188.56180831641268,266.6666666666667);
  size_scalar(c,clip,"_xscale",200);size_scalar(c,clip,"_yscale",300);dimension_write(c,clip,9,40);dimension_scale(c,clip,266.6666666666667,188.56180831641268);
  size_scalar(c,clip,"_rotation",0);size_scalar(c,clip,"_xscale",100);size_scalar(c,clip,"_yscale",100);
  FVM_Object *nested=child(c,clip,"nested",2);size_rect(c,nested,30,-10,5,5);size_scalar(c,root,"_xscale",0);
  dimension_write(c,clip,8,70);dimension_scale(c,clip,200,100);size_expect(c,clip,8,70);
  dimension_write(c,clip,8,std::numeric_limits<double>::quiet_NaN());dimension_scale(c,clip,200,100);
  dimension_write(c,clip,8,std::numeric_limits<double>::infinity());dimension_scale(c,clip,0,100);
  dimension_write(c,clip,8,70);FVM_Value target=size_object(clip),at,value,out;fvm_value_number(&at,8);fvm_value_number(&value,DBL_MAX);fvm_value_number(&out,77);
  assert(fvm_exec_display_property(c,1,&target,&at,&value,&out)==FVM_UNSUPPORTED && out.number==77);dimension_scale(c,clip,200,100);
  fvm_value_undefined(&value);assert(fvm_exec_display_property(c,1,&target,&at,&value,&out)==FVM_OK);value.type=FVM_VALUE_NULL;assert(fvm_exec_display_property(c,1,&target,&at,&value,&out)==FVM_OK);dimension_scale(c,clip,200,100);
  FVM_Object *flat=child(c,root,"flat",3);size_rect(c,flat,0,0,10,0);dimension_write(c,flat,8,40);dimension_scale(c,flat,0,0);
  assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);
}
#endif
