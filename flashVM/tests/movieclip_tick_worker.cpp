/* Actual OS workers stop before tree entry and during snapshot preparation after a native handler. */
#include "movieclip_tick_fixture.hpp"
#include "flashvm/worker.h"
struct WorkerTick { unsigned calls; };
static FVM_Result stop(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *raw)
{ WorkerTick *p=(WorkerTick *)raw;++p->calls;FVM_Worker *self=0;assert(fvm_worker_current(c,&self)==FVM_OK && fvm_worker_stop(self)==FVM_OK);fvm_value_undefined(out);return FVM_OK; }
static FVM_Result run(FVM_Context *c,const FVM_Value *input,FVM_Value *out,void *)
{
 assert(fvm_standard_install(c)==FVM_OK);FVM_Object *root=tick_root(c);WorkerTick p;p.calls=0;
 if(input->boolean){for(unsigned i=0;i<70;++i)tick_child(c,root,"child",i);assert(fvm_object_define_method_rooted(c,root,"onEnterFrame",stop,&p,0)==FVM_OK);}
 else {FVM_Worker *self=0;assert(fvm_worker_current(c,&self)==FVM_OK && fvm_worker_stop(self)==FVM_OK);}
 assert(fvm_movieclip_advance(c,root,4)==FVM_ERROR && p.calls==(input->boolean?1U:0U));tick_clean(c);assert(fvm_gc_collect(c,0)==FVM_OK);fvm_value_boolean(out,1);return FVM_OK;
}
int main()
{
 FVM_Config cfg;assert(fvm_config_init(&cfg)==FVM_OK);cfg.instruction_limit=0;
 for(unsigned i=0;i<2;++i){FVM_Worker *worker=0;FVM_Value in,out;fvm_value_boolean(&in,i);assert(fvm_worker_run(&cfg,run,&in,0,&worker)==FVM_OK);assert(fvm_worker_join(worker,&out)==FVM_OK && out.type==FVM_VALUE_BOOLEAN && out.boolean);fvm_worker_destroy(worker);}

}
