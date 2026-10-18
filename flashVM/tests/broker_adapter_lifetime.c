#include "flashvm/capability.h"
#include "flashvm/debug.h"
#include "flashvm/gc.h"
#include "flashvm/mutex.h"
#include "flashvm/serialization.h"
#include "flashvm/table.h"
#include "flashvm/transfer.h"
#include <assert.h>
#include <string.h>
int main(void)
{
    FVM_BrokerStats stats,unchanged;
    memset(&stats,0x5a,sizeof(stats));unchanged=stats;
    assert(fvm_broker_stats(0,&stats)==FVM_INVALID_ARGUMENT && !memcmp(&stats,&unchanged,sizeof(stats)));
#ifdef OS_PORTS
    {
        FVM_Config cfg;FVM_Runtime *rt;FVM_Context *owner,*client;FVM_ResourceBroker *broker;
        FVM_Channel *queue,*network;FVM_Mutex *mutex;FVM_Object *wrapper,*graph,*lock;
        FVM_Value root,value,edge;char *uri;unsigned i;
        assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
        assert(fvm_context_create(rt,&owner)==FVM_OK);assert(fvm_context_create(rt,&client)==FVM_OK);
        assert(fvm_broker_start("127.0.0.1","127.0.0.1",0,&broker)==FVM_OK);
        assert(fvm_context_set_broker(owner,broker)==FVM_OK);
        assert(fvm_broker_stats(broker,0)==FVM_INVALID_ARGUMENT);
        assert(fvm_broker_stats(broker,&stats)==FVM_OK && !stats.peer_contexts && !stats.adapter_contexts);
        assert(fvm_channel_create(&queue)==FVM_OK);assert(fvm_extensions_wrap_channel(owner,queue,&wrapper)==FVM_OK);
        assert(fvm_broker_export(owner,wrapper,&uri)==FVM_OK);assert(fvm_channel_open_network(client,uri,&network)==FVM_OK);
        assert(fvm_mutex_create(owner,&mutex)==FVM_OK);assert(fvm_extensions_wrap_mutex(owner,mutex,&lock)==FVM_OK);
        assert(fvm_object_create(owner,&graph)==FVM_OK);
        fvm_value_undefined(&root);root.type=FVM_VALUE_OBJECT;root.object=graph;
        assert(fvm_object_set(owner,graph,"self",&root)==FVM_OK);
        value=root;value.object=lock;assert(fvm_object_set(owner,graph,"lock",&value)==FVM_OK);
        assert(fvm_channel_send_value(owner,queue,&root,0)==FVM_OK);
        for(i=0;i<200;++i)
        {
            assert(fvm_debug_channel_message(client,network,0,&value)==FVM_OK);
            assert(fvm_gc_pin(client,value.object)==FVM_OK);assert(fvm_gc_collect(client,0)==FVM_OK);
            assert(fvm_object_get(client,value.object,"self",&edge)==FVM_OK && edge.object==value.object);
            assert(fvm_gc_unpin(client,value.object)==FVM_OK);assert(fvm_gc_collect(client,0)==FVM_OK);
            assert(fvm_broker_stats(broker,&stats)==FVM_OK && !stats.peer_contexts && !stats.adapter_contexts);
            assert(stats.exported_resources==2);
        }
        assert(fvm_channel_receive_value(client,network,&value,0)==FVM_OK);
        assert(fvm_object_get(client,value.object,"lock",&edge)==FVM_OK);
        assert(fvm_object_call(client,edge.object,"set",0,0,&root)==FVM_OK);
        assert(fvm_broker_stats(broker,&stats)==FVM_OK && stats.peer_contexts==1 && !stats.adapter_contexts && stats.adapter_requests>=201);
        assert(fvm_object_call(client,edge.object,"reset",0,0,&root)==FVM_OK);
        fvm_value_undefined(&root);root.type=FVM_VALUE_OBJECT;root.object=graph;
        for(i=0;i<200;++i)
        {
            assert(fvm_channel_send_value(owner,queue,&root,0)==FVM_OK);
            assert(fvm_channel_receive_value(client,network,&value,0)==FVM_OK);
            assert(fvm_gc_collect(client,0)==FVM_OK);
            assert(fvm_broker_stats(broker,&stats)==FVM_OK && stats.peer_contexts==1 && !stats.adapter_contexts && stats.exported_resources==2);
        }
        assert(stats.adapter_requests>=401);
        fvm_channel_release(network);fvm_channel_release(queue);fvm_mutex_release(mutex);
        fvm_broker_stop(broker);memset(&stats,0x5a,sizeof(stats));unchanged=stats;
        assert(fvm_broker_stats(broker,&stats)==FVM_ERROR && !memcmp(&stats,&unchanged,sizeof(stats)));
        fvm_broker_release(broker);fvm_serialized_free(uri);
        fvm_context_destroy(client);fvm_context_destroy(owner);fvm_runtime_destroy(rt);
    }
#endif
    return 0;
}
