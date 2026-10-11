#include "flashvm/channel.h"
#include "../../ports/channel_sync.h"
#include <stdlib.h>
#include <string.h>
typedef struct FVM_ChannelNode {
    FVM_Value value;
    char *owned_string;
    struct FVM_ChannelNode *next;
} FVM_ChannelNode;
struct FVM_Channel {
    FVM_PortSync *sync;
    FVM_ChannelNode *head;
    FVM_ChannelNode *tail;
    char *last_string;
    int closed;
    size_t pending;
    size_t capacity;
};
FVM_Result fvm_channel_create(FVM_Channel **out) {
    FVM_Channel *c;
    if (!out) return FVM_INVALID_ARGUMENT;
    *out=0;
    c=(FVM_Channel *)calloc(1,sizeof(*c));
    if (!c) return FVM_OUT_OF_MEMORY;
    c->sync=fvm_port_sync_create();
    if (!c->sync) { free(c); return FVM_OUT_OF_MEMORY; }
    *out=c;
    return FVM_OK;
}
void fvm_channel_close(FVM_Channel *c) {
    if (!c) return;
    fvm_port_sync_lock(c->sync);
    c->closed=1;
    fvm_port_sync_signal_all(c->sync);
    fvm_port_sync_unlock(c->sync);
}
void fvm_channel_destroy(FVM_Channel *c) {
    FVM_ChannelNode *n,*next;
    if (!c) return;
    fvm_channel_close(c);
    n=c->head;
    while (n) { next=n->next; free(n->owned_string); free(n); n=next; }
    free(c->last_string);
    fvm_port_sync_destroy(c->sync);
    free(c);
}
FVM_Result fvm_channel_send(FVM_Channel *c,const FVM_Value *v) {
    FVM_ChannelNode *n;
    size_t len;
    if (!c || !v || v->type==FVM_VALUE_OBJECT) return FVM_INVALID_ARGUMENT;
    n=(FVM_ChannelNode *)calloc(1,sizeof(*n));
    if (!n) return FVM_OUT_OF_MEMORY;
    n->value=*v;
    if (v->type==FVM_VALUE_STRING) {
        if (!v->string) { free(n); return FVM_INVALID_ARGUMENT; }
        len=strlen(v->string);
        n->owned_string=(char *)malloc(len+1);
        if (!n->owned_string) { free(n); return FVM_OUT_OF_MEMORY; }
        memcpy(n->owned_string,v->string,len+1);
        n->value.string=n->owned_string;
    }
    fvm_port_sync_lock(c->sync);
    while (!c->closed && c->capacity && c->pending >= c->capacity)
        fvm_port_sync_wait(c->sync);
    if (c->closed) {
        fvm_port_sync_unlock(c->sync);
        free(n->owned_string); free(n);
        return FVM_ERROR;
    }
    if (c->tail) c->tail->next=n; else c->head=n;
    c->tail=n;
    ++c->pending;
    fvm_port_sync_signal_all(c->sync);
    fvm_port_sync_unlock(c->sync);
    return FVM_OK;
}
FVM_Result fvm_channel_receive(FVM_Channel *c,FVM_Value *out) {
    FVM_ChannelNode *n;
    char *previous;
    if (!c || !out) return FVM_INVALID_ARGUMENT;
    fvm_port_sync_lock(c->sync);
    while (!c->head && !c->closed) fvm_port_sync_wait(c->sync);
    if (!c->head) { fvm_port_sync_unlock(c->sync); return FVM_ERROR; }
    n=c->head;
    c->head=n->next;
    if (!c->head) c->tail=0;
    --c->pending;
    fvm_port_sync_signal_all(c->sync);
    *out=n->value;
    previous=c->last_string;
    c->last_string=n->owned_string;
    fvm_port_sync_unlock(c->sync);
    free(previous);
    free(n);
    return FVM_OK;
}
FVM_Result fvm_channel_try_receive(FVM_Channel *c, FVM_Value *out) {
    FVM_ChannelNode *n;
    char *previous;
    FVM_Result result;
    if (!c || !out) return FVM_INVALID_ARGUMENT;
    fvm_port_sync_lock(c->sync);
    if (!c->head) {
        result = c->closed ? FVM_ERROR : FVM_NOT_FOUND;
        fvm_port_sync_unlock(c->sync);
        return result;
    }
    n = c->head;
    c->head = n->next;
    if (!c->head) c->tail = 0;
    --c->pending;
    fvm_port_sync_signal_all(c->sync);
    *out = n->value;
    previous = c->last_string;
    c->last_string = n->owned_string;
    fvm_port_sync_unlock(c->sync);
    free(previous);
    free(n);
    return FVM_OK;
}
FVM_Result fvm_channel_available(FVM_Channel *c,int *available) {
    if (!c || !available) return FVM_INVALID_ARGUMENT;
    fvm_port_sync_lock(c->sync);
    *available=(c->head!=0);
    fvm_port_sync_unlock(c->sync);
    return FVM_OK;
}

FVM_Result fvm_channel_set_capacity(FVM_Channel *c, size_t capacity) {
    if (!c) return FVM_INVALID_ARGUMENT;
    fvm_port_sync_lock(c->sync);
    c->capacity = capacity;
    fvm_port_sync_signal_all(c->sync);
    fvm_port_sync_unlock(c->sync);
    return FVM_OK;
}
FVM_Result fvm_channel_pending(FVM_Channel *c, size_t *pending) {
    if (!c || !pending) return FVM_INVALID_ARGUMENT;
    fvm_port_sync_lock(c->sync);
    *pending = c->pending;
    fvm_port_sync_unlock(c->sync);
    return FVM_OK;
}
