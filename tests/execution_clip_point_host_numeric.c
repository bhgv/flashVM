/* Host point methods preserve overflow propagation and inverse-then-apply cancellation. */
#include "flashvm/execution.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include <assert.h>
#include <float.h>
int main(void)
{
 FVM_Config cfg;FVM_Runtime *rt;unsigned native,reverse;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(native=0;native<2;++native)for(reverse=0;reverse<2;++reverse)
 {
  FVM_Context *c;FVM_Object *clip,*point;FVM_Value v,arg,out;FVM_Affine m={2,0,0,1,0,0};
  assert(fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"clip",1,&clip)==FVM_OK && fvm_gc_pin(c,clip)==FVM_OK && fvm_object_create(c,&point)==FVM_OK && fvm_gc_pin(c,point)==FVM_OK);
  if(reverse){m.xx=3;m.x0=1e16;}
  assert(fvm_movieclip_set_matrix(c,clip,&m)==FVM_OK);
  fvm_value_number(&v,reverse?1e16+2:DBL_MAX);assert(fvm_object_set(c,point,"x",&v)==FVM_OK);fvm_value_number(&v,0);assert(fvm_object_set(c,point,"y",&v)==FVM_OK);
  fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=point;
  assert((native?fvm_object_call(c,clip,reverse?"globalToLocal":"localToGlobal",1,&arg,&out):fvm_exec_clip_point(c,clip,reverse,&arg))==FVM_OK);
  assert(fvm_object_get(c,point,"x",&out)==FVM_OK && (reverse?out.number==1:out.number>DBL_MAX));
  assert(fvm_object_get(c,point,"y",&out)==FVM_OK && out.number==0 && fvm_gc_collect(c,0)==FVM_OK);
  fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);return 0;
}
