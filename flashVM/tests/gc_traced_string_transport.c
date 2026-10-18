#include "flashvm/gc.h"
#include "flashvm/transfer.h"
#include "flashvm/table.h"
#include "flashvm/capability.h"
#include "flashvm/extensions.h"
#include "flashvm/serialization.h"
#include "flashvm/worker.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static FVM_Result work(FVM_Context *c,const FVM_Value *input,FVM_Value *out,void *raw)
{
    unsigned short text[3]={'A',0,'B'};unsigned int i;FVM_Value value;FVM_GCStringStats stats;
    (void)input;(void)raw;
    assert(fvm_gc_string_mode(c,FVM_GC_STRINGS_TRACED)==FVM_OK);
    for(i=0;i<1000;++i)
    {
        assert(fvm_value_string_utf16(c,text,3,&value)==FVM_OK);
        assert(fvm_set_global(c,"s",&value)==FVM_OK);
        if(i%20==19)
        {
            assert(fvm_gc_collect(c,0)==FVM_OK);
            assert(fvm_gc_string_stats(c,&stats)==FVM_OK && stats.buffers==1);
            assert(fvm_get_global(c,"s",&value)==FVM_OK && !memcmp(value.string,"A\0B",3));
        }
    }
    fvm_value_number(out,1000);return FVM_OK;
}
int main(void)
{
    FVM_Config cfg;FVM_Runtime *rt;FVM_Context *source,*dest;FVM_Channel *channel,*proxy;
    FVM_Object *object;FVM_Value root,value,edge,out;FVM_GCStringStats stats;
    FVM_Worker *workers[4];unsigned short text[3]={'A',0,'B'};unsigned int i;int kind;
#ifdef OS_PORTS
    FVM_ResourceBroker *broker;FVM_Object *wrapper;char *uri;
#endif
    assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&source)==FVM_OK);assert(fvm_context_create(rt,&dest)==FVM_OK);
    assert(fvm_gc_string_mode(source,FVM_GC_STRINGS_TRACED)==FVM_OK);
    assert(fvm_gc_string_mode(dest,FVM_GC_STRINGS_TRACED)==FVM_OK);
    for(kind=0;kind<3;++kind)
    {
#ifndef OS_PORTS
        if(kind==2)continue;
#endif
        proxy=0;
        if(kind==1)
        {
            remove("gc-traced-transport.bin");
            assert(fvm_channel_open_file("gc-traced-transport.bin","rw",&channel)==FVM_OK);
        }
        else assert(fvm_channel_create(&channel)==FVM_OK);
#ifdef OS_PORTS
        if(kind==2)
        {
            assert(fvm_broker_start("127.0.0.1","127.0.0.1",0,&broker)==FVM_OK);
            assert(fvm_context_set_broker(source,broker)==FVM_OK);
            assert(fvm_extensions_wrap_channel(source,channel,&wrapper)==FVM_OK);
            assert(fvm_broker_export(source,wrapper,&uri)==FVM_OK);
            assert(fvm_channel_open_network(dest,uri,&proxy)==FVM_OK);
        }
#endif
        assert(fvm_object_create(source,&object)==FVM_OK);
        assert(fvm_value_string_utf16(source,text,3,&value)==FVM_OK);
        assert(fvm_object_set(source,object,"bytes",&value)==FVM_OK);
        assert(fvm_table_set_index(source,object,0,&value)==FVM_OK);
        fvm_value_undefined(&root);root.type=FVM_VALUE_OBJECT;root.object=object;
        assert(fvm_object_set(source,object,"self",&root)==FVM_OK);
        assert(fvm_channel_send_value(source,channel,&root,0)==FVM_OK);
        assert(fvm_gc_collect(source,0)==FVM_OK);
        assert(fvm_gc_string_stats(source,&stats)==FVM_OK && !stats.buffers);
        assert(fvm_channel_receive_value(dest,proxy?proxy:channel,&out,0)==FVM_OK);
        assert(fvm_gc_pin(dest,out.object)==FVM_OK);assert(fvm_gc_collect(dest,0)==FVM_OK);
        assert(fvm_object_get(dest,out.object,"self",&edge)==FVM_OK && edge.object==out.object);
        assert(fvm_object_get(dest,out.object,"bytes",&value)==FVM_OK && value.string_size==3 && !memcmp(value.string,"A\0B",3));
        assert(fvm_table_get_index(dest,out.object,0,&value)==FVM_OK && !memcmp(value.string,"A\0B",3));
        assert(fvm_gc_unpin(dest,out.object)==FVM_OK);assert(fvm_gc_collect(dest,0)==FVM_OK);
        assert(fvm_gc_string_stats(dest,&stats)==FVM_OK && !stats.buffers);
        if(proxy)fvm_channel_release(proxy);
        fvm_channel_destroy(channel);
        if(kind==1)assert(!remove("gc-traced-transport.bin"));
#ifdef OS_PORTS
        if(kind==2)
        {
            assert(fvm_context_set_broker(source,0)==FVM_OK);
            fvm_broker_stop(broker);fvm_broker_release(broker);fvm_serialized_free(uri);
        }
#endif
    }
    fvm_value_undefined(&value);
    for(i=0;i<4;++i)assert(fvm_worker_run(&cfg,work,&value,0,&workers[i])==FVM_OK);
    for(i=0;i<4;++i)
    {
        assert(fvm_worker_join(workers[i],&out)==FVM_OK && out.number==1000);
        fvm_worker_destroy(workers[i]);
    }
    fvm_context_destroy(dest);fvm_context_destroy(source);fvm_runtime_destroy(rt);return 0;
}
