#include "flashvm/channel_value.h"
#include <assert.h>
int main(void) {
    FVM_ChannelValue a,b;
    FVM_Value in,out;
    assert(fvm_channel_value_create(&a)==FVM_OK);
    assert(fvm_channel_value_copy(&b,&a)==FVM_OK);
    fvm_channel_value_release(&a);
    fvm_value_number(&in,42);
    assert(fvm_channel_value_send(&b,&in)==FVM_OK);
    assert(fvm_channel_value_receive(&b,&out)==FVM_OK);
    assert(out.type==FVM_VALUE_NUMBER && out.number==42);
    fvm_channel_value_release(&b);
    return 0;
}
