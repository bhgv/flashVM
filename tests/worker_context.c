#include "flashvm/worker.h"
#include <assert.h>
static FVM_Result inspect(FVM_Context *c,const FVM_Value *in,FVM_Value *out,void *ud) {
    FVM_Worker *w=0;int stop=1;(void)in;(void)ud;
    assert(fvm_worker_current(c,&w)==FVM_OK && w);
    assert(fvm_worker_join(w,0)==FVM_INVALID_ARGUMENT);
    assert(fvm_worker_stop_requested(w,&stop)==FVM_OK && !stop);
    fvm_value_number(out,42);return FVM_OK;
}
int main(void) {
    FVM_Config cfg;FVM_Runtime *rt;FVM_Context *c;FVM_Worker *w;FVM_Value in,out;unsigned int i;
    assert(fvm_config_init(&cfg)==FVM_OK);fvm_value_undefined(&in);
    for(i=0;i<40;++i) {
        assert(fvm_worker_run(&cfg,inspect,&in,0,&w)==FVM_OK);
        assert(fvm_worker_join(w,&out)==FVM_OK && out.number==42);
        fvm_worker_destroy(w);
    }
    assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);assert(fvm_context_create(rt,&c)==FVM_OK);
    assert(fvm_worker_current(c,&w)==FVM_NOT_FOUND && !w);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
