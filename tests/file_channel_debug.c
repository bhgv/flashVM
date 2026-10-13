#include "flashvm/debug.h"
#include "flashvm/transfer.h"
#include "flashvm/gc.h"
#include "flashvm/table.h"
#include "flashvm/worker.h"
#include "../ports/thread_port.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static FVM_Result read_worker(FVM_Context *c,const FVM_Value *in,FVM_Value *out,void *ud)
{
    (void)c;(void)in;
    return fvm_channel_receive_owned((FVM_Channel *)ud,out,1);
}
int main(void)
{
    const char *path="file-channel-debug.bin";
    FVM_Config cfg;FVM_Runtime *rt;FVM_Context *c;FVM_Channel *ch;
    FVM_Object *o;FVM_Value v,peek,edge,in,out;FVM_Worker *worker;
    FVM_WorkerState state;FVM_DebugChannel info;unsigned long n;size_t count;char byte;
    remove(path);
    assert(fvm_config_init(&cfg)==FVM_OK);
    assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK);
    assert(fvm_channel_open_file(path,"rw",&ch)==FVM_OK);
    assert(fvm_object_create(c,&o)==FVM_OK);
    fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=o;
    assert(fvm_object_set(c,o,"self",&v)==FVM_OK);
    assert(fvm_channel_send_value(c,ch,&v,0)==FVM_OK);
    assert(fvm_value_string(&v,"second")==FVM_OK);
    assert(fvm_channel_send(ch,&v)==FVM_OK);
    assert(fvm_debug_channel_freeze(ch,1)==FVM_OK);
    assert(fvm_debug_channel_state(ch,&info)==FVM_OK && info.pending==2 && info.frozen && !info.closed && !info.capacity);
    assert(fvm_channel_file_read(ch,&byte,1,&count)==FVM_NOT_FOUND && count==0);
    assert(fvm_channel_try_receive(ch,&out)==FVM_NOT_FOUND);
    assert(fvm_channel_receive_value(c,ch,&out,0)==FVM_NOT_FOUND);
    assert(fvm_debug_channel_message(c,ch,1,&peek)==FVM_OK && !strcmp(peek.string,"second"));
    assert(fvm_debug_channel_message(c,ch,2,&out)==FVM_NOT_FOUND);
    assert(fvm_debug_channel_message(c,ch,0,&peek)==FVM_OK);
    assert(fvm_gc_pin(c,peek.object)==FVM_OK);
    assert(fvm_gc_collect(c,&n)==FVM_OK);
    assert(fvm_object_get(c,peek.object,"self",&edge)==FVM_OK && edge.object==peek.object);
    assert(fvm_debug_channel_freeze(ch,0)==FVM_OK);
    assert(fvm_channel_receive_value(c,ch,&out,0)==FVM_OK && out.object!=peek.object);
    assert(fvm_object_get(c,out.object,"self",&edge)==FVM_OK && edge.object==out.object);
    assert(fvm_gc_unpin(c,peek.object)==FVM_OK);
    assert(fvm_debug_channel_state(ch,&info)==FVM_OK && info.pending==1 && !info.frozen);
    assert(fvm_debug_channel_freeze(ch,1)==FVM_OK);
    fvm_value_undefined(&in);
    assert(fvm_worker_run(&cfg,read_worker,&in,ch,&worker)==FVM_OK);
    fvm_port_thread_sleep_us(10000);
    assert(fvm_worker_state(worker,&state)==FVM_OK && state!=FVM_WORKER_FINISHED);
    assert(fvm_debug_channel_message(c,ch,0,&peek)==FVM_OK && !strcmp(peek.string,"second"));
    assert(fvm_debug_channel_freeze(ch,0)==FVM_OK);
    assert(fvm_worker_join(worker,&out)==FVM_OK && !strcmp(out.string,"second"));
    fvm_worker_destroy(worker);
    assert(fvm_debug_channel_message(c,ch,0,&out)==FVM_NOT_FOUND);
    assert(fvm_debug_channel_state(ch,&info)==FVM_OK && !info.pending);
    /* Malformed record inspections, including skipping earlier records, never consume bytes. */
    assert(fvm_channel_file_write(ch,"bad",3)==FVM_OK);
    info.pending=123;info.identity=456;
    assert(fvm_debug_channel_state(ch,&info)==FVM_INVALID_ARGUMENT && info.pending==123 && info.identity==456);
    assert(fvm_debug_channel_message(c,ch,0,&out)==FVM_INVALID_ARGUMENT);
    assert(fvm_debug_channel_message(c,ch,1,&out)==FVM_INVALID_ARGUMENT);
    assert(fvm_channel_file_read(ch,&byte,1,&count)==FVM_OK && count==1 && byte=='b');
    assert(fvm_debug_channel_freeze(ch,1)==FVM_OK);
    assert(fvm_worker_run(&cfg,read_worker,&in,ch,&worker)==FVM_OK);
    fvm_port_thread_sleep_us(10000);
    assert(fvm_worker_state(worker,&state)==FVM_OK && state!=FVM_WORKER_FINISHED);
    fvm_channel_close(ch);
    assert(fvm_debug_channel_state(ch,&info)==FVM_OK && info.closed && info.frozen && !info.pending);
    assert(fvm_worker_join(worker,&out)==FVM_ERROR);
    fvm_worker_destroy(worker);
    assert(fvm_channel_receive_owned(ch,&out,0)==FVM_ERROR);
    assert(fvm_debug_channel_message(c,ch,0,&out)==FVM_ERROR);
    fvm_channel_destroy(ch);
    assert(fvm_channel_open_file(path,"w",&ch)==FVM_OK);
    assert(fvm_debug_channel_message(c,ch,0,&out)==FVM_ERROR);
    assert(fvm_debug_channel_state(ch,&info)==FVM_ERROR);
    fvm_channel_destroy(ch);
    assert(!remove(path));
    assert(fvm_channel_open_file(path,"rw",&ch)==FVM_OK);
    {
        size_t records=65537,bytes=records*16,i;unsigned char *data=(unsigned char *)calloc(bytes,1);
        assert(data);for(i=0;i<records;++i)memcpy(data+i*16,"FVMG\0\0\0\7",8);
        assert(fvm_channel_file_write(ch,data,bytes-16)==FVM_OK);
        assert(fvm_debug_channel_state(ch,&info)==FVM_OK && info.pending==65536);
        assert(fvm_channel_file_write(ch,data+bytes-16,16)==FVM_OK);free(data);
        info.pending=987;info.identity=555;
        assert(fvm_debug_channel_state(ch,&info)==FVM_UNSUPPORTED && info.pending==987 && info.identity==555);
        assert(fvm_channel_file_read(ch,&byte,1,&count)==FVM_OK && count==1 && byte=='F');
    }
    fvm_channel_destroy(ch);
    assert(!remove(path));assert(fvm_channel_open_file(path,"rw",&ch)==FVM_OK);
    {
        unsigned char headers[32]={0};memcpy(headers,"FVMG\0\0\0\7",8);memcpy(headers+16,headers,8);
        headers[24]=3;headers[25]=255;headers[26]=255;headers[27]=240;
        assert(fvm_channel_file_write(ch,headers,32)==FVM_OK);
        info.pending=987;info.identity=555;
        assert(fvm_debug_channel_state(ch,&info)==FVM_UNSUPPORTED && info.pending==987 && info.identity==555);
        assert(fvm_channel_file_read(ch,&byte,1,&count)==FVM_OK && count==1 && byte=='F');
    }
    fvm_channel_destroy(ch);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);
    assert(!remove(path));return 0;
}
