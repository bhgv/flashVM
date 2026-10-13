#include "flashvm/capability.h"
#include "flashvm/exception.h"
#include "flashvm/debug.h"
#include "flashvm/extensions.h"
#include "flashvm/transfer.h"
#include "flashvm/serialization.h"
#include "flashvm/worker.h"
#include "../ports/thread_port.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifdef OS_PORTS
typedef struct Task
{
    FVM_Channel *channel;
    FVM_ExceptionInfo fault;
    int has_fault;
} Task;
static FVM_Result receive(FVM_Context *c,const FVM_Value *in,FVM_Value *out,void *ud)
{
    Task *task=(Task *)ud;FVM_Object *wrapper=0;FVM_Result result;(void)in;
    assert(fvm_debug_set_location(c,"network-worker.as",42)==FVM_OK);
    assert(fvm_extensions_wrap_channel(c,task->channel,&wrapper)==FVM_OK);
    result=fvm_object_call(c,wrapper,"receive",0,0,out);
    if(result==FVM_EXCEPTION)
    {
        assert(fvm_exception_info(c,&task->fault)==FVM_OK);task->has_fault=1;
    }
    return result;
}
#endif
int main(void)
{
#ifdef OS_PORTS
    FVM_Config cfg;FVM_Runtime *rt;FVM_Context *owner,*client;FVM_ResourceBroker *broker;
    FVM_Channel *queue,*network,*file,*remote_file;FVM_Object *wrapper;
    FVM_DebugChannel local,state;FVM_DebugResourceError errors[64];FVM_Value value,input,out;
    FVM_Worker *worker;FVM_WorkerState status;Task task;char *uri,*file_uri;size_t count;
    assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&owner)==FVM_OK);assert(fvm_context_create(rt,&client)==FVM_OK);
    assert(fvm_broker_start("127.0.0.1","127.0.0.1",0,&broker)==FVM_OK);
    assert(fvm_context_set_broker(owner,broker)==FVM_OK);
    assert(fvm_channel_create(&queue)==FVM_OK);assert(fvm_channel_set_capacity(queue,4)==FVM_OK);
    assert(fvm_extensions_wrap_channel(owner,queue,&wrapper)==FVM_OK);
    assert(fvm_broker_export(owner,wrapper,&uri)==FVM_OK);
    assert(fvm_channel_open_network(client,uri,&network)==FVM_OK);
    fvm_value_number(&value,7);assert(fvm_channel_send(queue,&value)==FVM_OK);
    assert(fvm_debug_channel_freeze(network,1)==FVM_OK);
    assert(fvm_debug_channel_state(queue,&local)==FVM_OK && local.frozen && local.pending==1);
    assert(fvm_debug_channel_state(network,&state)==FVM_OK && state.frozen && state.pending==1 && state.capacity==4 && state.identity==fvm_channel_identity(network));
    assert(fvm_channel_receive_value(client,network,&out,0)==FVM_NOT_FOUND);
    assert(fvm_channel_try_receive(queue,&out)==FVM_NOT_FOUND);
    fvm_value_number(&value,9);assert(fvm_channel_send_value(client,network,&value,0)==FVM_OK);
    assert(fvm_debug_channel_state(network,&state)==FVM_OK && state.pending==2 && state.frozen);
    fvm_value_undefined(&input);task.channel=network;task.has_fault=0;assert(fvm_worker_run(&cfg,receive,&input,&task,&worker)==FVM_OK);
    fvm_port_thread_sleep_us(10000);assert(fvm_worker_state(worker,&status)==FVM_OK && status!=FVM_WORKER_FINISHED);
    assert(fvm_debug_channel_freeze(network,0)==FVM_OK);
    assert(fvm_worker_join(worker,&out)==FVM_OK && out.number==7);fvm_worker_destroy(worker);
    assert(fvm_channel_receive_value(client,network,&out,0)==FVM_OK && out.number==9);
    assert(fvm_debug_channel_state(network,&state)==FVM_OK && !state.pending && !state.frozen);
    remove("network-debug-file.bin");assert(fvm_channel_open_file("network-debug-file.bin","rw",&file)==FVM_OK);
    fvm_value_number(&value,11);assert(fvm_channel_send(file,&value)==FVM_OK);
    assert(fvm_extensions_wrap_channel(owner,file,&wrapper)==FVM_OK);
    assert(fvm_broker_export(owner,wrapper,&file_uri)==FVM_OK);
    assert(fvm_channel_open_network(client,file_uri,&remote_file)==FVM_OK);
    assert(fvm_debug_channel_freeze(remote_file,1)==FVM_OK);
    assert(fvm_debug_channel_state(remote_file,&state)==FVM_OK && state.frozen && state.pending==1 && !state.capacity);
    assert(fvm_channel_receive_value(client,remote_file,&out,0)==FVM_NOT_FOUND);
    assert(fvm_debug_channel_freeze(remote_file,0)==FVM_OK);
    assert(fvm_channel_receive_value(client,remote_file,&out,0)==FVM_OK && out.number==11);
    assert(fvm_debug_channel_state(remote_file,&state)==FVM_OK && !state.pending);
    assert(fvm_debug_channel_freeze(network,1)==FVM_OK);
    task.has_fault=0;assert(fvm_worker_run(&cfg,receive,&input,&task,&worker)==FVM_OK);
    fvm_port_thread_sleep_us(10000);assert(fvm_worker_state(worker,&status)==FVM_OK && status!=FVM_WORKER_FINISHED);
    fvm_channel_close(queue);
    assert(fvm_worker_join(worker,&out)==FVM_EXCEPTION);fvm_worker_destroy(worker);
    assert(task.has_fault && task.fault.code==FVM_RESOURCE_CLOSED && task.fault.line==42 && !strcmp(task.fault.source,"network-worker.as") && (task.fault.flags & FVM_EXCEPTION_NETWORK) && !strcmp(task.fault.address,uri));
    assert(fvm_debug_channel_state(network,&state)==FVM_OK && state.closed && state.frozen);
    assert(fvm_debug_channel_freeze(network,0)==FVM_ERROR);
    fvm_broker_stop(broker);state.pending=987;state.identity=555;
    assert(fvm_debug_channel_state(network,&state)==FVM_ERROR && state.pending==987 && state.identity==555);
    assert(fvm_debug_resource_errors(errors,64,&count,1)==FVM_OK && count>0);
    assert(strstr(errors[count-1].address,"fvm://"));
    fvm_channel_release(network);fvm_channel_release(remote_file);
    fvm_context_destroy(client);fvm_context_destroy(owner);fvm_runtime_destroy(rt);
    fvm_channel_release(queue);fvm_channel_release(file);fvm_broker_release(broker);
    fvm_serialized_free(uri);fvm_serialized_free(file_uri);assert(!remove("network-debug-file.bin"));
#endif
    return 0;
}
