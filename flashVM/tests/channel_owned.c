#include "flashvm/channel.h"
#include <assert.h>
#include <string.h>
int main(void) {
    FVM_Channel *c;FVM_Value in,a,b;
    assert(fvm_channel_create(&c)==FVM_OK);
    assert(fvm_channel_set_capacity(c,1)==FVM_OK);
    assert(fvm_value_string(&in,"first")==FVM_OK);
    assert(fvm_channel_try_send(c,&in)==FVM_OK);
    assert(fvm_channel_try_send(c,&in)==FVM_NOT_FOUND);
    assert(fvm_channel_receive_owned(c,&a,0)==FVM_OK);
    assert(fvm_value_string(&in,"second")==FVM_OK);
    assert(fvm_channel_send(c,&in)==FVM_OK);
    assert(fvm_channel_receive_owned(c,&b,1)==FVM_OK);
    assert(strcmp(a.string,"first")==0 && strcmp(b.string,"second")==0);
    fvm_channel_close(c);
    assert(fvm_channel_try_send(c,&in)==FVM_ERROR);
    assert(fvm_channel_receive_owned(c,&in,0)==FVM_ERROR);
    fvm_channel_destroy(c);
    assert(strcmp(a.string,"first")==0); /* Independent from channel lifetime. */
    fvm_channel_message_release(&a);fvm_channel_message_release(&b);
    assert(a.type==FVM_VALUE_UNDEFINED && b.type==FVM_VALUE_UNDEFINED);
    return 0;
}
