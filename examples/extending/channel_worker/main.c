/* C89: worker gets a retained channel through userdata.
 * Caller MUST join worker before releasing its retained handle. */
#include "flashvm/channel_value.h"
#include "flashvm/worker.h"
#include <assert.h>
static FVM_Result task(FVM_Context *ctx, const FVM_Value *input,
                       FVM_Value *output, void *userdata) {
    FVM_ChannelValue *channel=(FVM_ChannelValue *)userdata;
    FVM_Value message;
    FVM_Result r;
    (void)ctx; (void)input;
    r=fvm_channel_value_receive(channel,&message);
    if (r==FVM_OK && message.type==FVM_VALUE_NUMBER)
        fvm_value_number(output,message.number+1);
    return r;
}
int main(void) {
    FVM_ChannelValue owner,worker_channel;
    FVM_Config config;
    FVM_Value input,message,result;
    FVM_Worker *worker=0;
    assert(fvm_channel_value_create(&owner)==FVM_OK);
    assert(fvm_channel_value_copy(&worker_channel,&owner)==FVM_OK);
    assert(fvm_config_init(&config)==FVM_OK);
    fvm_value_undefined(&input);
    assert(fvm_worker_run(&config,task,&input,&worker_channel,&worker)==FVM_OK);
    fvm_value_number(&message,41);
    assert(fvm_channel_value_send(&owner,&message)==FVM_OK);
    assert(fvm_worker_join(worker,&result)==FVM_OK);
    assert(result.type==FVM_VALUE_NUMBER && result.number==42);
    fvm_worker_destroy(worker);
    fvm_channel_value_release(&worker_channel);
    fvm_channel_value_release(&owner);
    return 0;
}
