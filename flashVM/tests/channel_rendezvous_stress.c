/* Multiple native producers/consumers must acknowledge each offer exactly once. */
#include "flashvm/channel.h"
#include "../ports/thread_port.h"
#include "../ports/channel_sync.h"
#include <stdio.h>
#define TRANSFERS 400
/* Caller owns every job and the channel through all native joins. */
typedef struct Job {
    FVM_Channel *channel;
    FVM_PortSync *sync;
    unsigned long *sum;
    int sending;
    int value;
    int failed;
} Job;
/* Exercise concurrent sender-stack acknowledgements without sharing VM heaps. */
static void run(void *user)
{
    Job *job = (Job *)user;
    FVM_Value value;
    int index;
    for (index = 0; index < TRANSFERS; index++)
    {
        if (job->sending)
        {
            fvm_value_number(&value, job->value);
            if (fvm_channel_send(job->channel, &value) != FVM_OK)
                job->failed = 1;
        }
        else
        {
            if (fvm_channel_receive_owned(job->channel, &value, 1) != FVM_OK)
                job->failed = 1;
            else
            {
                fvm_port_sync_lock(job->sync);
                *job->sum += (unsigned long)value.number;
                fvm_port_sync_unlock(job->sync);
                fvm_channel_message_release(&value);
            }
        }
    }
}
/* Two producers and two consumers share exactly one zero-buffer rendezvous queue. */
int main(void)
{
    FVM_Channel *channel = 0;
    FVM_PortSync *sync;
    FVM_PortThread *threads[4];
    FVM_ChannelWaitState state;
    Job jobs[4];
    unsigned long sum = 0;
    unsigned int index;
    if (fvm_channel_create(&channel) != FVM_OK || fvm_channel_set_rendezvous(channel, 1) != FVM_OK)
        return 1;
    sync = fvm_port_sync_create();
    if (!sync)
        return 2;
    for (index = 0; index < 4; index++)
    {
        jobs[index].channel = channel;
        jobs[index].sync = sync;
        jobs[index].sum = &sum;
        jobs[index].sending = index < 2;
        jobs[index].value = (int)index + 1;
        jobs[index].failed = 0;
        threads[index] = fvm_port_thread_start(run, &jobs[index]);
        if (!threads[index])
            return 3;
    }
    for (index = 0; index < 4; index++)
    {
        fvm_port_thread_join(threads[index]);
        if (jobs[index].failed)
            return 4;
    }
    if (sum != TRANSFERS * 3UL || fvm_channel_wait_state(channel, &state) != FVM_OK ||
        state.pending || state.waiting_senders || state.waiting_receivers)
        return 5;
    fvm_port_sync_destroy(sync);
    fvm_channel_destroy(channel);
    puts("PASS 800 rendezvous transfers with four native threads");
    return 0;
}
