#include "flashvm/channel.h"
#include <string.h>
#include <stdio.h>
int main(void) {
    FVM_Channel *c=0;
    FVM_Value in,out;
    int available=0;
    char text[16]="hello";
    if (fvm_channel_create(&c)!=FVM_OK) return 1;
    fvm_value_string(&in,text);
    if (fvm_channel_send(c,&in)!=FVM_OK) return 2;
    strcpy(text,"other");
    if (fvm_channel_available(c,&available)!=FVM_OK || !available) return 3;
    if (fvm_channel_receive(c,&out)!=FVM_OK || strcmp(out.string,"hello")) return 4;
    fvm_value_number(&in,42.0);
    if (fvm_channel_send(c,&in)!=FVM_OK) return 5;
    fvm_channel_close(c);
    if (fvm_channel_receive(c,&out)!=FVM_OK || out.number!=42.0) return 6;
    if (fvm_channel_receive(c,&out)!=FVM_ERROR) return 7;
    if (fvm_channel_send(c,&in)!=FVM_ERROR) return 8;
    fvm_channel_destroy(c);
    puts("channel test passed");
    return 0;
}
