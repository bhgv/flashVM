/* Copyright (c) 2026 bhgv
 * EN: Actual heartbeat/frame algorithms under deterministic short sends and monotonic clock.
 * RU: Настоящие heartbeat/frame algorithms с deterministic short sends и monotonic clock.
 */
#define phywrite capture
#define clock_gettime fixtureclock
#include "../../client/heartbeat.c"
#include "../../client/mux_loop.c"
#undef phywrite
#undef clock_gettime

static double milliseconds; /* Controlled monotonic fixture clock. */
static unsigned count; /* Accepted transport bytes. */
static uchar bytes[128]; /* Exact serialized transport output. */
static int blocked; /* Would-block without effects. */


/* EN/RU: Retain active checks in every native build mode.
 * Сохранить активные проверки во всех native build modes. */
static void check(int valid, char *why)
{
    if (!valid)
    {
        fprint(2, "heartbeat frames: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Supply controlled clock without elapsed wall-time assumptions.
 * Предоставить controlled clock без wall-time предположений. */
int fixtureclock(clockid_t id, struct timespec *out)
{
    USED(id);
    out->tv_sec = (time_t)(milliseconds/1000);
    out->tv_nsec = (long)(milliseconds-out->tv_sec*1000)*1000000;
    return 0;
}


/* EN/RU: Accept at most two bytes or EAGAIN; substitute only the native PHY write boundary.
 * Принять максимум два bytes либо EAGAIN; подменяется только native PHY write boundary. */
PhyTransfer capture(RemotePhy *phy, const void *input, size_t size)
{
    USED(phy);
    if (blocked)
    {
        PhyTransfer out = {0, PHY_AGAIN, PHY_PREFIX};
        return out;
    }
    unsigned n = size > 2 ? 2 : size;
    check(count+n <= sizeof(bytes), "output capacity");
    memcpy(bytes+count, input, n);
    count += n;
    PhyTransfer out = {n, PHY_OK, PHY_PREFIX};
    return out;
}


/* EN/RU: Encode a real Rflush using original codec for the permanently reserved probe tag.
 * Encode настоящий Rflush исходным codec для постоянно reserved probe tag. */
static int reply(StyxMux *mux)
{
    Fcall response;
    memset(&response, 0, sizeof(response));
    response.type = Rflush;
    response.tag = 2*mux->limit+1;
    mux->want = convS2M(&response, mux->receive, 256);
    return dispatch(mux);
}


/* EN/RU: Check send/reply budgets, no frame interleave, late/duplicate reply and huge clock jumps.
 * Проверить send/reply budgets, serialization, late/duplicate reply и huge clock jumps. */
int main(void)
{
    StyxMux mux;
    StyxFlight flight;
    StyxFlight *slots[1] = {nil};
    uchar receive[256], decode[256], request[256], answer[256];
    memset(&mux, 0, sizeof(mux));
    memset(&flight, 0, sizeof(flight));
    mux.limit = 1;
    mux.timeout = 1000;
    mux.slots = slots;
    mux.receive = receive;
    mux.decode = decode;
    mux.beat.idle = 10;
    mux.beat.reply = 20;
    mux.beat.misses = 3;
    mux.beat.due = 100;
    milliseconds = 100;
    check(styxbeattick(&mux, 100) == 21 && mux.beat.active, "probe admission");
    check(!reply(&mux), "unsent probe accepted reply");
    blocked = 1;
    sendframe(&mux);
    check(!mux.beat.sent && !count, "EAGAIN fabricated send");
    milliseconds = 120;
    styxbeattick(&mux, 120);
    check(mux.beat.notice == STYX_LINK_STALLED && !mux.failed, "send stall classification");
    blocked = 0;
    sendframe(&mux);
    Fcall message;
    memset(&message, 0, sizeof(message));
    message.type = Tread;
    message.tag = 1;
    message.fid = 2;
    message.count = 1;
    flight.request = request;
    flight.answer.bytes = answer;
    flight.length = convS2M(&message, request, sizeof(request));
    flight.bound = 256;
    flight.tag = 1;
    flight.flush = 2;
    flight.deadline = 10000;
    check(!pthread_cond_init(&flight.condition, nil), "condition");
    slots[0] = &flight;
    for (unsigned i = 0; i < 4; i++)
    {
        sendframe(&mux);
    }
    check(mux.beat.sent == 9 && !flight.sent && count == 9 && !memcmp(bytes, mux.beat.bytes, 9), "partial probe interleaved");
    check(mux.beat.deadline == 140 && !mux.beat.missed, "reply phase budget not captured on complete send");
    milliseconds = 140;
    styxbeattick(&mux, 140);
    check(mux.beat.notice == STYX_LINK_SUSPECT && !mux.failed, "reply suspicion");
    styxbeattraffic(&mux);
    check(mux.beat.notice == STYX_LINK_READY && mux.beat.active && mux.beat.deadline == 160, "ordinary activity recovery dropped probe lease");
    check(reply(&mux) && !mux.beat.active, "valid probe response");
    check(!reply(&mux), "duplicate probe response");
    for (unsigned i = 0; i < 20 && flight.sent < flight.length; i++)
    {
        sendframe(&mux);
    }
    check(flight.sent == flight.length && !memcmp(bytes+9, request, flight.length), "ordinary frame bytes");
    flight.watching = 1;
    check(deadlines(&mux, 1000000) == -1 && !flight.flushing && !flight.done, "watch read hit ordinary deadline");
    StyxCancel *cancel = styxcancelcreate();
    check(cancel != nil, "watch cancellation token");
    styxcancelrequest(cancel);
    flight.cancel = cancel;
    check(deadlines(&mux, 1000000) == 1001 && flight.flushing && !flight.done && !mux.failed, "watch cancellation lost bounded flush grace");
    flight.cancel = nil;
    styxcancelfree(cancel);
    mux.beat.idle = 0;
    check(styxbeattick(&mux, 1000) == -1, "disabled new probe");
    mux.beat.idle = 10;
    mux.beat.due = 1000;
    styxbeattick(&mux, 1000);
    styxbeattick(&mux, 1e15);
    check(mux.failed && mux.terminal == STYX_LINK_TIMEOUT && flight.done, "large clock jump did not bound retirement");
    pthread_cond_destroy(&flight.condition);
    print("PASS deterministic heartbeat frames and budgets\n");
    return 0;
}
