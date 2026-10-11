/* C89 example: a copied handle refers to the same synchronized queue. */
#include "flashvm/channel_value.h"
#include <assert.h>
int main(void) {
    FVM_ChannelValue producer,consumer;
    FVM_Value sent,received;
    assert(fvm_channel_value_create(&producer)==FVM_OK);
    assert(fvm_channel_value_copy(&consumer,&producer)==FVM_OK);
    fvm_value_number(&sent,17);
    assert(fvm_channel_value_send(&producer,&sent)==FVM_OK);
    assert(fvm_channel_value_receive(&consumer,&received)==FVM_OK);
    assert(received.type==FVM_VALUE_NUMBER && received.number==17);
    fvm_channel_value_release(&consumer);
    fvm_channel_value_release(&producer);
    return 0;
}
