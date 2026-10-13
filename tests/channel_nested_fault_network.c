#include "flashvm/capability.h"
#include "flashvm/debug.h"
#include "flashvm/exception.h"
#include "flashvm/extensions.h"
#include "flashvm/mutex.h"
#include "flashvm/serialization.h"
#include "flashvm/transfer.h"
#include <assert.h>
#include <string.h>
#ifdef OS_PORTS
static FVM_Result inspect(FVM_Context *c,FVM_Object *self,unsigned n,const FVM_Value *args,FVM_Value *out,void *raw)
{(void)self;(void)n;(void)args;return fvm_debug_channel_message(c,(FVM_Channel *)raw,0,out);}
#endif
int main(void)
{
#ifdef OS_PORTS
    FVM_Config cfg;FVM_Runtime *rt;FVM_Context *owner,*client;FVM_ResourceBroker *broker;
    FVM_Channel *queue,*network,*inner_channel;FVM_Mutex *mutex;FVM_Object *wrapper,*proxy,*inner;
    FVM_Value value,out;FVM_ExceptionInfo fault;FVM_DebugChannel state;char *uri;int kind;
    assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&owner)==FVM_OK);assert(fvm_context_create(rt,&client)==FVM_OK);
    assert(fvm_broker_start("127.0.0.1","127.0.0.1",0,&broker)==FVM_OK);
    assert(fvm_context_set_broker(owner,broker)==FVM_OK);
    for(kind=0;kind<2;++kind)
    {
        assert(fvm_channel_create(&queue)==FVM_OK);
        assert(fvm_extensions_wrap_channel(owner,queue,&wrapper)==FVM_OK);
        assert(fvm_broker_export(owner,wrapper,&uri)==FVM_OK);
        assert(fvm_channel_open_network(client,uri,&network)==FVM_OK);
        assert(fvm_extensions_wrap_channel(client,network,&proxy)==FVM_OK);
        assert(fvm_object_define_method_rooted(client,proxy,"inspect",inspect,network,0)==FVM_OK);
        if(kind==0)
        {
            assert(fvm_mutex_create(owner,&mutex)==FVM_OK);
            assert(fvm_extensions_wrap_mutex(owner,mutex,&inner)==FVM_OK);
        }
        else
        {
            assert(fvm_channel_create(&inner_channel)==FVM_OK);
            assert(fvm_extensions_wrap_channel(owner,inner_channel,&inner)==FVM_OK);
        }
        fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=inner;
        assert(fvm_channel_send_value(owner,queue,&value,0)==FVM_OK);
        if(kind==0)assert(fvm_mutex_close(mutex)==FVM_OK);else fvm_channel_close(inner_channel);
        assert(fvm_debug_set_location(client,"nested-capability.as",29)==FVM_OK);
        assert(fvm_object_call(client,proxy,"inspect",0,0,&out)==FVM_EXCEPTION);
        assert(fvm_exception_info(client,&fault)==FVM_OK && fault.code==FVM_RESOURCE_CLOSED);
        assert(fault.line==29 && !strcmp(fault.source,"nested-capability.as") && !strcmp(fault.address,uri));
        assert(fvm_debug_channel_state(network,&state)==FVM_OK && state.pending==1 && !state.closed);
        assert(fvm_exception_clear(client)==FVM_OK);
        assert(fvm_object_call(client,proxy,"receive",0,0,&out)==FVM_EXCEPTION);
        assert(fvm_exception_info(client,&fault)==FVM_OK && fault.code==FVM_RESOURCE_CLOSED);
        assert(fault.line==29 && !strcmp(fault.source,"nested-capability.as") && (fault.flags&FVM_EXCEPTION_NETWORK));
        assert(fvm_debug_channel_state(network,&state)==FVM_OK && !state.pending && !state.closed);
        assert(fvm_exception_clear(client)==FVM_OK);
        fvm_value_number(&value,19);assert(fvm_channel_send(queue,&value)==FVM_OK);
        assert(fvm_channel_receive_value(client,network,&out,0)==FVM_OK && out.number==19);
        fvm_channel_release(network);fvm_channel_release(queue);
        if(kind==0)fvm_mutex_release(mutex);else fvm_channel_release(inner_channel);
        fvm_serialized_free(uri);
    }
    fvm_broker_stop(broker);fvm_broker_release(broker);
    fvm_context_destroy(client);fvm_context_destroy(owner);fvm_runtime_destroy(rt);
#endif
    return 0;
}
