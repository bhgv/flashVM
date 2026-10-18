/* C89 embedding background state has no managed object ownership requirements. */
#include "flashvm/stage.h"
#include <assert.h>
int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    FVM_RasterColor color = {12,34,56,78};
    FVM_RasterColor out = {1,2,3,4};
    assert(fvm_config_init(&cfg)==FVM_OK);
    assert(fvm_runtime_create(&cfg,&runtime)==FVM_OK);
    assert(fvm_context_create(runtime,&context)==FVM_OK);
    assert(fvm_stage_get_background(context,&out)==FVM_NOT_FOUND && out.r==1 && out.a==4);
    assert(fvm_stage_set_background(context,color)==FVM_OK);
    assert(fvm_stage_get_background(context,&out)==FVM_OK && out.r==12 && out.g==34 && out.b==56 && out.a==78);
    assert(fvm_stage_get_background(0,&out)==FVM_INVALID_ARGUMENT);
    assert(fvm_stage_get_background(context,0)==FVM_INVALID_ARGUMENT);
    assert(fvm_stage_set_background(0,color)==FVM_INVALID_ARGUMENT);
    assert(fvm_stage_clear_background(context)==FVM_OK);
    assert(fvm_stage_get_background(context,&out)==FVM_NOT_FOUND && out.r==12 && out.a==78);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    return 0;
}
