/* Copyright (c) 2026 bhgv
 * EN: Portable regression for native FIFO, mixed ALT, expiry and clunk.
 * RU: Переносимая проверка FIFO, смешанного ALT, истечения срока и clunk.
 */
#include "styxrpc/channel_scheduler.h"
#include <stdio.h>
#include <string.h>

typedef struct Operation
{
    StyxRpcChannelFid fid;  /* One fresh operation per open. */
    unsigned char send[96]; /* Contiguous request storage borrowed by fid. */
    unsigned char recv[96]; /* Contiguous reply storage borrowed by fid. */
} Operation;

static const StyxRpcChannelGrant grants[] = {{1, "(i)"}, {2, "(s)"}}; /* Sync int and buffered string. */


/* EN: Issue a canonical operation through the regular fid write path.
 * RU: Послать каноническую операцию через штатную запись fid. */
static int issue(StyxRpcChannelScheduler *scheduler, Operation *operation, const StyxRpcSelection *selection)
{
    unsigned char scratch[96], frame[96];
    size_t size;
    operation->fid = (StyxRpcChannelFid){
        .grants = grants, .grant_count = 2, .request = operation->send, .request_capacity = sizeof(operation->send),
        .reply = operation->recv, .reply_capacity = sizeof(operation->recv),
        .admit = styxrpc_scheduler_admit, .cancel = styxrpc_scheduler_cancel, .context = scheduler
    };
    return styxrpc_channel_request(grants, 2, selection, scratch, sizeof(scratch), frame, sizeof(frame), &size) ==
        STYXRPC_OK && styxrpc_channel_fid_write(&operation->fid, 0, frame, size) == STYXRPC_OK;
}


/* EN: Decode the committed choice against the original operation and compare its value.
 * RU: Проверить готовую ветку относительно исходной операции и сравнить значение. */
static int answer(const Operation *operation, unsigned selected, const void *value, size_t size)
{
    unsigned char frame[96];
    size_t used;
    StyxRpcChannelResult result;
    return styxrpc_channel_fid_read(&operation->fid, 0, frame, sizeof(frame), &used) == STYXRPC_OK &&
        styxrpc_channel_result(grants, 2, &operation->fid.selection, frame, used, &result) == STYXRPC_OK &&
        result.selected == selected && result.size == size && (!size || !memcmp(result.value, value, size));
}


/* EN: Return the deterministic monotonic time supplied by this test.
 * RU: Вернуть управляемое монотонное время теста. */
static uint64_t clock_ms(void *context)
{
    return *(const uint64_t *)context;
}


/* EN: Verify synchronous grants never pair with themselves or with a closed fid.
 * RU: Проверить, что синхронные ветки не связываются с собой или закрытым fid. */
static int rendezvous(StyxRpcChannelScheduler *scheduler)
{
    unsigned char value[16], wrapped[24];
    size_t size, extent;
    if (styxrpc_value_integer(STYXRPC_VALUE_INT, 42, value, sizeof(value), &size) != STYXRPC_OK ||
        fvm_srecord_write(STYXRPC_CHANNEL_ELEMENT_TAG, value, size, wrapped, sizeof(wrapped), &extent) != FVM_S_OK)
    {
        return 0;
    }

    Operation closed, self, receiver, sender;
    const StyxRpcSelection send = {.count = 1, .guards = {{1, 0, value, size}}};
    const StyxRpcSelection select_self = {.mode = STYXRPC_CHANNEL_TRY, .count = 2,
        .guards = {{1, 0, value, size}, {1, 1, NULL, 0}}};
    const StyxRpcSelection receive = {.mode = STYXRPC_CHANNEL_ALT, .count = 2,
        .guards = {{2, 1, NULL, 0}, {1, 1, NULL, 0}}};
    if (!issue(scheduler, &self, &select_self) || !answer(&self, 0, NULL, 0) ||
        !issue(scheduler, &closed, &send) || !closed.fid.pending)
    {
        return 0;
    }
    styxrpc_channel_fid_close(&closed.fid);
    return issue(scheduler, &receiver, &receive) && receiver.fid.pending &&
        issue(scheduler, &sender, &send) && answer(&sender, 1, NULL, 0) && answer(&receiver, 2, wrapped, extent) &&
        !scheduler->used;
}


/* EN: Verify buffered FIFO, no early WAIT reply and expiry without a message.
 * RU: Проверить буферную FIFO, ожидание без раннего ответа и завершение по сроку. */
static int fifo_and_wait(StyxRpcChannelScheduler *scheduler, uint64_t *now)
{
    unsigned char value[2][24], child[16];
    size_t size[2], length;
    for (size_t i = 0; i < 2; ++i)
    {
        if (fvm_srecord_write(STYXRPC_VALUE_STRING, i ? "b" : "a", 1, child, sizeof(child), &length) != FVM_S_OK ||
            fvm_srecord_write(STYXRPC_CHANNEL_ELEMENT_TAG, child, length, value[i], sizeof(value[i]), &size[i]) != FVM_S_OK)
        {
            return 0;
        }
    }
    Operation sender[2], receiver[2], waiting;
    const StyxRpcSelection receive = {.count = 1, .guards = {{2, 1, NULL, 0}}};
    const StyxRpcSelection wait = {.mode = STYXRPC_CHANNEL_WAIT, .count = 1, .guards = {{2, 1, NULL, 0}}};
    const StyxRpcSelection first = {.count = 1, .guards = {{2, 0, value[0], size[0]}}};
    const StyxRpcSelection second = {.count = 1, .guards = {{2, 0, value[1], size[1]}}};
    if (!issue(scheduler, &sender[0], &first) || !answer(&sender[0], 1, NULL, 0) ||
        !issue(scheduler, &sender[1], &second) || !sender[1].fid.pending ||
        !issue(scheduler, &receiver[0], &receive) || !answer(&receiver[0], 1, value[0], size[0]) ||
        !answer(&sender[1], 1, NULL, 0) ||
        !issue(scheduler, &receiver[1], &receive) || !answer(&receiver[1], 1, value[1], size[1]) ||
        !issue(scheduler, &waiting, &wait) || !waiting.fid.pending)
    {
        return 0;
    }
    *now += 39;
    styxrpc_scheduler_progress(scheduler);
    if (!waiting.fid.pending)
    {
        return 0;
    }
    ++*now;
    styxrpc_scheduler_progress(scheduler);
    return answer(&waiting, 0, NULL, 0) && !scheduler->used;
}


/* EN: Run both policies on one scheduler without Styx or a platform clock.
 * RU: Проверить обе политики на одном планировщике без Styx и системных часов. */
int main(void)
{
    unsigned char arena[24];
    size_t lengths[1];
    StyxRpcChannelQueue queues[] = {
        {.grant = grants[0]},
        {.grant = grants[1], .bytes = arena, .sizes = lengths, .capacity = 1, .stride = sizeof(arena)}
    };
    StyxRpcChannelScheduler scheduler;
    uint64_t now = 100;
    if (!styxrpc_scheduler_init(&scheduler, queues, 2, clock_ms, &now) || !rendezvous(&scheduler))
    {
        fputs("scheduler rendezvous, mixed ALT or clunk failed\n", stderr);
        return 1;
    }
    if (!fifo_and_wait(&scheduler, &now))
    {
        fputs("scheduler buffered FIFO or WAIT failed\n", stderr);
        return 1;
    }
    return 0;
}
