/* Legacy/malformed private state is handled without publishing an invalid query result. */
#include "flashvm/execution.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "../src/core/runtime.hpp"
#include <cassert>
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *clip=0;FVM_Value out,v;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"root",1,&clip)==FVM_OK);
 FVM_Object *data=clip->resource_result.object;data->properties.erase("detached");
 assert(fvm_exec_clip_detached(c,clip,&out)==FVM_OK && out.type==FVM_VALUE_BOOLEAN && !out.boolean);
 fvm_value_undefined(&v);data->properties["detached"]=v;fvm_value_number(&out,73);
 assert(fvm_exec_clip_detached(c,clip,&out)==FVM_INVALID_ARGUMENT && out.type==FVM_VALUE_NUMBER && out.number==73);
 fvm_value_number(&v,2);data->properties["detached"]=v;
 assert(fvm_exec_clip_detached(c,clip,&out)==FVM_INVALID_ARGUMENT && out.number==73);
 fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
