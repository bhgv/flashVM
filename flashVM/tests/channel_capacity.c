#include "flashvm/channel.h"
#include <stdio.h>
int main(void) {
    FVM_Channel *c = 0;
    FVM_Value v, out;
    size_t pending = 99;
    if (fvm_channel_create(&c) != FVM_OK) return 1;
    if (fvm_channel_set_capacity(c, 2) != FVM_OK) return 2;
    fvm_value_number(&v, 1);
    if (fvm_channel_send(c, &v) != FVM_OK) return 3;
    if (fvm_channel_send(c, &v) != FVM_OK) return 4;
    if (fvm_channel_pending(c, &pending) != FVM_OK || pending != 2) return 5;
    if (fvm_channel_try_receive(c, &out) != FVM_OK) return 6;
    if (fvm_channel_pending(c, &pending) != FVM_OK || pending != 1) return 7;
    fvm_channel_close(c);
    if (fvm_channel_receive(c, &out) != FVM_OK) return 8;
    if (fvm_channel_pending(c, &pending) != FVM_OK || pending != 0) return 9;
    fvm_channel_destroy(c);
    puts("channel capacity test passed");
    return 0;
}
