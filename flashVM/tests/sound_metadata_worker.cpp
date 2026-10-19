/* Every metadata route polls an actual owning worker, even with numeric limits disabled. */
#include "sound_metadata_fixture.hpp"
#include "flashvm/worker.h"
static FVM_Result metadata_worker(FVM_Context *c,const FVM_Value *input,FVM_Value *out,void *)
{
    Bytes swf=metadata_movie();assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);FVM_Worker *self=0;
    assert(fvm_worker_current(c,&self)==FVM_OK && fvm_worker_stop(self)==FVM_OK);
    assert(metadata_query(c,(unsigned)input->number,true)==FVM_ERROR && !c->avm1_execution_depth);
    fvm_value_boolean(out,1);return FVM_OK;
}
int main()
{
    FVM_Config cfg;assert(fvm_config_init(&cfg)==FVM_OK);cfg.instruction_limit=0;
    for(unsigned route=0;route<6;++route){FVM_Value input,out;fvm_value_number(&input,route);FVM_Worker *worker=0;
        assert(fvm_worker_run(&cfg,metadata_worker,&input,0,&worker)==FVM_OK && fvm_worker_join(worker,&out)==FVM_OK && out.boolean);fvm_worker_destroy(worker);}
}
