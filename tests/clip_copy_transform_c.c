/* Strict C89 calls use native clip identities without bytecode or a script Transform object. */
#include "flashvm/execution.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include <assert.h>
#include <math.h>
int main(void)
{
 FVM_Config cfg;FVM_Runtime *rt;FVM_Context *c;FVM_Object *source,*dest;FVM_Value value;
 FVM_Affine pose={2,0,0,-3,17,19},result;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);
 assert(fvm_movieclip_create(c,"source",1,&source)==FVM_OK && fvm_gc_pin(c,source)==FVM_OK && fvm_movieclip_create(c,"dest",1,&dest)==FVM_OK && fvm_gc_pin(c,dest)==FVM_OK);
 assert(fvm_movieclip_set_matrix(c,source,&pose)==FVM_OK);fvm_value_number(&value,37);assert(fvm_object_set(c,source,"_alpha",&value)==FVM_OK);
 assert(fvm_exec_clip_copy_transform(c,dest,source)==FVM_OK && fvm_movieclip_get_matrix(c,dest,&result)==FVM_OK && fabs(result.xx-2)<1e-9 && fabs(result.yy+3)<1e-9 && result.x0==17 && result.y0==19);
 assert(fvm_object_get(c,dest,"_alpha",&value)==FVM_OK && fabs(value.number-37)<1e-9 && fvm_gc_collect(c,0)==FVM_OK);
 fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
