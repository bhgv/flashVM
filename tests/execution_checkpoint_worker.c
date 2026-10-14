#include "flashvm/execution.h"
#include "flashvm/function.h"
#include "flashvm/worker.h"
#include "flashvm/avm1.h"
#include "flashvm/gc.h"
#include <assert.h>
static FVM_Result generated(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *raw)
{
    FVM_Worker *worker=0;(void)frame;(void)raw;(void)out;
    assert(fvm_exec_checkpoint(c,2,7,0)==FVM_OK);
    assert(fvm_worker_current(c,&worker)==FVM_OK && fvm_worker_stop(worker)==FVM_OK);
    assert(fvm_exec_checkpoint(c,0,8,0)==FVM_OK);
    return fvm_exec_checkpoint(c,0,9,FVM_EXEC_POLL);
}
static FVM_Result worker_body(FVM_Context *c,const FVM_Value *input,FVM_Value *out,void *raw)
{
    FVM_Object *function=0;FVM_Value result;FVM_AVM1TargetInfo target;const unsigned char code[]={0};
    (void)input;(void)raw;
    assert(fvm_function_create_avm1(c,code,sizeof(code),0,0,0,&function)==FVM_OK);
    assert(fvm_function_set_body(c,function,generated,0,0)==FVM_OK);
    assert(fvm_function_invoke(c,function,0,0,0,&result)==FVM_ERROR);
    assert(fvm_exec_checkpoint(c,0,0,FVM_EXEC_POLL)==FVM_INVALID_ARGUMENT);
    target.struct_size=sizeof(target);assert(fvm_avm1_get_target_info(c,&target)==FVM_OK && !target.active);
    assert(fvm_gc_collect(c,0)==FVM_OK);fvm_value_number(out,1);return FVM_OK;
}
int main(void)
{
    FVM_Config cfg;FVM_Worker *worker=0;FVM_Value input,out;
    assert(fvm_config_init(&cfg)==FVM_OK);fvm_value_undefined(&input);
    assert(fvm_worker_run_suspended(&cfg,worker_body,&input,0,&worker)==FVM_OK);
    assert(fvm_worker_join(worker,&out)==FVM_OK && out.type==FVM_VALUE_NUMBER && out.number==1);
    fvm_worker_destroy(worker);return 0;
}
