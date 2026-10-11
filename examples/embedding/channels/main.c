/* C89 example: a typed FIFO channel can transfer values without sharing VM objects. */
#include "flashvm/channel.h"
#include <stdio.h>
int main(void) {
    FVM_Channel *channel=0;
    FVM_Value sent,received;
    if (fvm_channel_create(&channel)!=FVM_OK) return 1;
    fvm_value_number(&sent,123.0);
    if (fvm_channel_send(channel,&sent)!=FVM_OK) return 2;
    if (fvm_channel_receive(channel,&received)!=FVM_OK) return 3;
    printf("Received: %.0f\n",received.number);
    fvm_channel_close(channel);
    fvm_channel_destroy(channel);
    return 0;
}
