#include "flashvm/channel.h"
#include <assert.h>
int main(void) {
    FVM_Channel *a=0, *b;
    FVM_Value in, out;
    assert(fvm_channel_create(&a)==FVM_OK);
    assert(fvm_channel_retain(a)==FVM_OK);
    b=a; /* second owner, e.g. a worker context */
    fvm_channel_destroy(a); /* original owner relinquishes its handle */
    fvm_value_number(&in,42);
    assert(fvm_channel_send(b,&in)==FVM_OK);
    assert(fvm_channel_receive(b,&out)==FVM_OK);
    assert(out.type==FVM_VALUE_NUMBER && out.number==42);
    fvm_channel_release(b);
    return 0;
}
