#include "flashvm/capability.h"
#include "flashvm/debug.h"
#include "flashvm/exception.h"
#include "flashvm/mutex.h"
#include "flashvm/worker.h"
#include "flashvm/serialization.h"
#include "../ports/thread_port.h"
#include <assert.h>
#include <string.h>
#ifdef OS_PORTS
typedef struct Task
{
    FVM_ExceptionInfo fault;
    unsigned long context;
    int has_fault;
} Task;
static FVM_Result acquire(FVM_Context *c,const FVM_Value *in,FVM_Value *out,void *raw)
{
    Task *task=(Task *)raw;FVM_Object *proxy;FVM_Result r;
    task->context=fvm_context_identity(c);
    assert(fvm_debug_set_location(c,"remote-wait.as",27)==FVM_OK);
    assert(fvm_broker_import(c,in->string,&proxy)==FVM_OK);
    r=fvm_object_call(c,proxy,"set",0,0,out);
    if(r==FVM_OK)assert(fvm_object_call(c,proxy,"reset",0,0,out)==FVM_OK);
    else if(r==FVM_EXCEPTION)
    {
        assert(fvm_exception_info(c,&task->fault)==FVM_OK);task->has_fault=1;
    }
    return r;
}
static void wait_visible(const char *uri)
{
    FVM_DebugMutexDetail rows[16];size_t count,i;int attempt;
    for(attempt=0;attempt<5000;++attempt)
    {
        assert(fvm_debug_mutex_detail_snapshot(rows,16,&count)==FVM_OK);
        for(i=0;i<count;++i)
            if(rows[i].remote && rows[i].state.waiter && !strcmp(rows[i].address,uri))
            {
                assert(rows[i].state.line==27 && !strcmp(rows[i].state.source,"remote-wait.as"));
                assert(!rows[i].state.deadlocked && !rows[i].state.owner);
                return;
            }
        fvm_port_thread_sleep_us(1000);
    }
    assert(0);
}
static void no_waiters(void)
{
    FVM_DebugMutexDetail rows[16];size_t count,i;
    assert(fvm_debug_mutex_detail_snapshot(rows,16,&count)==FVM_OK);
    for(i=0;i<count;++i)assert(!rows[i].state.waiter);
}
#endif
int main(void)
{
    size_t count=0;
    assert(fvm_debug_mutex_detail_snapshot(0,1,&count)==FVM_INVALID_ARGUMENT);
    assert(fvm_debug_mutex_detail_snapshot(0,0,0)==FVM_INVALID_ARGUMENT);
#ifdef OS_PORTS
    {
        FVM_Config cfg;FVM_Runtime *rt;FVM_Context *owner;FVM_ResourceBroker *broker;
        FVM_Mutex *mutex;FVM_Object *wrapper;FVM_Worker *worker;FVM_Value in,out;
        FVM_DebugMutexDetail sentinel,unchanged;char *uri;Task task;int mode,held;
        assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
        assert(fvm_context_create(rt,&owner)==FVM_OK);
        assert(fvm_broker_start("127.0.0.1","127.0.0.1",0,&broker)==FVM_OK);
        assert(fvm_context_set_broker(owner,broker)==FVM_OK);
        assert(fvm_mutex_create(owner,&mutex)==FVM_OK);
        assert(fvm_extensions_wrap_mutex(owner,mutex,&wrapper)==FVM_OK);
        assert(fvm_broker_export(owner,wrapper,&uri)==FVM_OK);
        assert(fvm_value_string(&in,uri)==FVM_OK);
        for(mode=0;mode<3;++mode)
        {
            assert(fvm_debug_set_location(owner,"authority.as",12)==FVM_OK);
            assert(fvm_mutex_set(owner,mutex,0)==FVM_OK);
            memset(&task,0,sizeof(task));
            assert(fvm_worker_run(&cfg,acquire,&in,&task,&worker)==FVM_OK);
            wait_visible(uri);
            memset(&sentinel,0x5a,sizeof(sentinel));unchanged=sentinel;
            assert(fvm_debug_mutex_detail_snapshot(&sentinel,1,&count)==FVM_NOT_FOUND && count>=2);
            assert(!memcmp(&sentinel,&unchanged,sizeof(sentinel)));
            if(mode==0)assert(fvm_mutex_reset(owner,mutex)==FVM_OK);
            else if(mode==1)assert(fvm_worker_stop(worker)==FVM_OK);
            else assert(fvm_mutex_close(mutex)==FVM_OK);
            assert(fvm_worker_join(worker,&out)==(mode?FVM_EXCEPTION:FVM_OK));
            if(mode)
            {
                assert(task.has_fault && task.fault.code==(mode==1?FVM_RESOURCE_CANCELLED:FVM_RESOURCE_CLOSED));
                assert(task.fault.line==27 && !strcmp(task.fault.source,"remote-wait.as"));
                assert(task.fault.thread==task.context && !strcmp(task.fault.address,uri));
            }
            fvm_worker_destroy(worker);no_waiters();
            if(mode==1)assert(fvm_mutex_reset(owner,mutex)==FVM_OK);
            if(mode<2)assert(fvm_mutex_touch(mutex,&held)==FVM_OK && !held);
        }
        fvm_mutex_release(mutex);fvm_broker_stop(broker);fvm_broker_release(broker);
        fvm_context_destroy(owner);fvm_runtime_destroy(rt);fvm_serialized_free(uri);
    }
#endif
    assert(fvm_debug_mutex_detail_snapshot(0,0,&count)==FVM_OK && !count);
    return 0;
}
