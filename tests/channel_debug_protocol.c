#include "flashvm/debug.h"
#include "../src/core/channel_packet.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static FVM_Result request(void *raw,FVM_Context *c,int op,const unsigned char *input,size_t size,
                           unsigned char **output,size_t *length,int blocking)
{
    int mode=*(int *)raw;(void)c;(void)blocking;
    if(output)*output=0;if(length)*length=0;
    if(op==7)
    {
        assert(size==1 && input[0]<=1);
        if(mode==1){*output=(unsigned char *)calloc(1,1);assert(*output);*length=1;}
        return FVM_OK;
    }
    if(op!=8)return FVM_OK;
    if(mode==4)return FVM_OUT_OF_MEMORY;
    if(mode==5)return FVM_NOT_FOUND;
    *length=mode==1?11:12;
    if(mode==2)return FVM_OK;
    *output=(unsigned char *)calloc(12,1);assert(*output);
    (*output)[3]=4;(*output)[7]=5;(*output)[11]=(unsigned char)(mode==3?4:3);
    return FVM_OK;
}
static void release(void *raw){(void)raw;}
int main(void)
{
    int mode=0;FVM_Channel *ch;FVM_DebugChannel state;
    assert(fvm_channel_create_remote(&mode,request,release,"adapter:test",&ch)==FVM_OK);
    assert(fvm_debug_channel_state(ch,&state)==FVM_OK && state.pending==4 && state.capacity==5 && state.closed && state.frozen && state.identity==fvm_channel_identity(ch));
    for(mode=1;mode<=5;++mode)
    {
        FVM_Result expected=mode==4?FVM_OUT_OF_MEMORY:mode==5?FVM_NOT_FOUND:FVM_ERROR;
        state.pending=987;state.identity=555;
        assert(fvm_debug_channel_state(ch,&state)==expected && state.pending==987 && state.identity==555);
    }
    mode=1;assert(fvm_debug_channel_freeze(ch,1)==FVM_ERROR);
    mode=0;assert(fvm_debug_channel_freeze(ch,1)==FVM_OK);
    assert(fvm_debug_channel_freeze(ch,0)==FVM_OK);
    fvm_channel_release(ch);return 0;
}
