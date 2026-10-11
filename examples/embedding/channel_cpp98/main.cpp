/* C++98 example: RAII wrapper over the stable C89 channel API. */
#include "flashvm/channel_value.h"
#include <cassert>
class ChannelOwner {
    FVM_ChannelValue value_;
    ChannelOwner(const ChannelOwner &);
    ChannelOwner &operator=(const ChannelOwner &);
public:
    ChannelOwner() { value_.channel=0; assert(fvm_channel_value_create(&value_)==FVM_OK); }
    ~ChannelOwner() { fvm_channel_value_release(&value_); }
    FVM_ChannelValue *get() { return &value_; }
};
int main() {
    ChannelOwner owner;
    FVM_ChannelValue other;
    FVM_Value input,output;
    assert(fvm_channel_value_copy(&other,owner.get())==FVM_OK);
    fvm_value_boolean(&input,1);
    assert(fvm_channel_value_send(&other,&input)==FVM_OK);
    assert(fvm_channel_value_receive(owner.get(),&output)==FVM_OK);
    assert(output.type==FVM_VALUE_BOOLEAN && output.boolean);
    fvm_channel_value_release(&other);
    return 0;
}
