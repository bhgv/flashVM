#include "flashvm/worker.h"
#include "../ports/channel_sync.h"
#include <assert.h>
typedef struct Data {
    FVM_Worker *target;
    FVM_PortSync *sync;
    int entered;
} Data;
static FVM_Result target(FVM_Context *c,const FVM_Value *in,FVM_Value *out,void *ud) {
    Data *d=(Data *)ud;
    (void)c;(void)in;
    assert(fvm_worker_join(d->target,0)==FVM_INVALID_ARGUMENT);
    fvm_port_sync_lock(d->sync);
    while(d->entered<2) fvm_port_sync_wait(d->sync);
    fvm_port_sync_unlock(d->sync);
    fvm_value_number(out,42);return FVM_OK;
}
static FVM_Result joiner(FVM_Context *c,const FVM_Value *in,FVM_Value *out,void *ud) {
    Data *d=(Data *)ud;
    (void)c;(void)in;
    fvm_port_sync_lock(d->sync);
    ++d->entered;fvm_port_sync_signal_all(d->sync);
    fvm_port_sync_unlock(d->sync);
    return fvm_worker_join(d->target,out);
}
int main(void) {
    FVM_Config cfg;FVM_Worker *joins[2];Data d;FVM_Value v,results[2];
    assert(fvm_config_init(&cfg)==FVM_OK);fvm_value_undefined(&v);
    d.sync=fvm_port_sync_create();d.entered=0;d.target=0;assert(d.sync);
    assert(fvm_worker_run_suspended(&cfg,target,&v,&d,&d.target)==FVM_OK);
    assert(fvm_worker_start(d.target)==FVM_OK);
    assert(fvm_worker_run(&cfg,joiner,&v,&d,&joins[0])==FVM_OK);
    assert(fvm_worker_run(&cfg,joiner,&v,&d,&joins[1])==FVM_OK);
    assert(fvm_worker_join_all(joins,2,results)==FVM_OK);
    assert(results[0].number==42 && results[1].number==42);
    fvm_worker_destroy(joins[0]);fvm_worker_destroy(joins[1]);
    fvm_worker_destroy(d.target);fvm_port_sync_destroy(d.sync);return 0;
}
