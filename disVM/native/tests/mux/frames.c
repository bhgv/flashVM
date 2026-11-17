/* Copyright (c) 2026 bhgv
 * EN: Deterministic partial-send/flush fences using the actual native owner algorithm.
 * RU: Детерминированные partial-send/flush fences на настоящем native owner algorithm.
 */
#define phywrite capture
#include "../../client/mux_loop.c"
#undef phywrite

static uchar sentbytes[512]; /* Captured exact output order. */
static unsigned nsent; /* Bytes accepted by the fake nonblocking transport. */
static int blocked; /* Controlled would-block without effects. */


/* EN/RU: Keep semantic checks active in release builds.
 * Сохранить semantic checks в release builds. */
static void check(int valid, char *why)
{
    if (!valid)
    {
        fprint(2, "native mux frames: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Accept at most two bytes, never infer atomic frame writes from real TCP.
 * Принять максимум два bytes, без предположения atomic frame writes. */
PhyTransfer capture(RemotePhy *phy, const void *bytes, size_t size)
{
    USED(phy);
    if (blocked)
    {
        PhyTransfer out = {0, PHY_AGAIN, PHY_PREFIX};
        return out;
    }
    unsigned count = size > 2 ? 2 : size;
    check(nsent+count <= sizeof(sentbytes), "capture bounds");
    memcpy(sentbytes+nsent, bytes, count);
    nsent += count;
    PhyTransfer out = {count, PHY_OK, PHY_PREFIX};
    return out;
}


/* EN/RU: Prepare a real encoded Tread with caller-owned fixture storage.
 * Подготовить настоящий encoded Tread в caller-owned fixture storage. */
static void prepare(StyxMux *mux, StyxFlight *flight, unsigned slot)
{
    memset(flight, 0, sizeof(*flight));
    check(!pthread_cond_init(&flight->condition, nil), "native condition");
    flight->request = malloc(256);
    flight->answer.bytes = malloc(256);
    check(flight->request != nil && flight->answer.bytes != nil, "buffers");
    flight->slot = slot;
    flight->tag = slot+1;
    flight->flush = mux->limit+slot+1;
    flight->bound = 256;
    flight->deadline = 100000;
    Fcall request;
    memset(&request, 0, sizeof(request));
    request.type = Tread;
    request.fid = slot+2;
    request.tag = flight->tag;
    request.count = 2;
    flight->length = convS2M(&request, flight->request, 256);
    mux->slots[slot] = flight;
}


/* EN/RU: Encode one original reply or flush reply into the actual dispatcher input.
 * Encode original/flush reply в настоящий dispatcher input. */
static int answer(StyxMux *mux, StyxFlight *flight, int flushing)
{
    Fcall reply;
    memset(&reply, 0, sizeof(reply));
    reply.type = flushing ? Rflush : Rread;
    reply.tag = flushing ? flight->flush : flight->tag;
    reply.count = 2;
    reply.data = "ok";
    mux->want = convS2M(&reply, mux->receive, 256);
    return dispatch(mux);
}


/* EN/RU: Retire fixture allocations only after the native table detached them.
 * Убрать fixture allocations после detachment из native table. */
static void release(StyxFlight *flight)
{
    free(flight->request);
    free(flight->answer.bytes);
    pthread_cond_destroy(&flight->condition);
}


/* EN/RU: Verify unsent cancellation, partial frame serialization and known-ack retention.
 * Проверить unsent cancellation, partial frame order и known-ack retention. */
int main(void)
{
    StyxMux mux;
    StyxFlight first, second;
    StyxFlight *slots[2] = {nil, nil};
    uchar receive[256], decode[256];
    memset(&mux, 0, sizeof(mux));
    check(!pthread_mutex_init(&mux.lock, nil), "native lock");
    pthread_mutex_lock(&mux.lock);
    mux.slots = slots;
    mux.limit = 2;
    mux.timeout = 1000;
    mux.receive = receive;
    mux.decode = decode;
    StyxCancel *cancel = styxcancelcreate();
    check(cancel && styxcancelbind(cancel, &mux), "token admission");
    styxcancelunbind(cancel);
    check(!styxcancelbind(cancel, &mux), "retired token reused for a later operation");
    styxcancelrequest(cancel);
    check(styxcancelled(cancel), "late token request lost");
    styxcancelfree(cancel);
    prepare(&mux, &first, 0);
    blocked = 1;
    sendframe(&mux);
    check(!first.answer.attempted && !nsent, "would-block fabricated effects");
    first.deadline = 0;
    deadlines(&mux, 1);
    check(first.done && first.answer.cancelled && !first.answer.attempted && !mux.sending, "unsent cancel needs no flush");
    release(&first);
    blocked = 0;
    prepare(&mux, &first, 0);
    prepare(&mux, &second, 1);
    mux.next = 0;
    sendframe(&mux);
    check(first.sent == 2 && first.answer.attempted, "partial original send");
    check(!answer(&mux, &first, 0), "premature acknowledgement accepted");
    first.deadline = 0;
    deadlines(&mux, 1);
    for (unsigned i = 0; i < 128 && first.sent < first.length; i++)
    {
        sendframe(&mux);
    }
    check(first.sent == first.length && !second.sent, "original frame interleaved");
    sendframe(&mux);
    check(first.flushsent == 2 && !second.sent, "flush did not get its reserved slot");
    check(answer(&mux, &first, 0) && first.original && !first.done, "original acknowledgement not preserved during partial flush");
    check(!answer(&mux, &first, 0), "duplicate original accepted before Rflush");
    for (unsigned i = 0; i < 128 && first.flushsent < sizeof(first.flushbytes); i++)
    {
        sendframe(&mux);
    }
    check(first.flushsent == sizeof(first.flushbytes) && !second.sent, "flush frame interleaved");
    check(nsent == first.length+sizeof(first.flushbytes) && !memcmp(sentbytes, first.request, first.length) &&
        !memcmp(sentbytes+first.length, first.flushbytes, sizeof(first.flushbytes)), "wrong exact frame order");
    check(answer(&mux, &first, 1) && first.done && first.answer.status == 1 && !first.answer.cancelled,
        "Rflush discarded known original acknowledgement");
    styxmuxbroken(&mux);
    check(second.done && !second.answer.attempted && second.answer.status < 0, "unsent unrelated failure invented effects");
    release(&first);
    release(&second);
    pthread_mutex_unlock(&mux.lock);
    pthread_mutex_destroy(&mux.lock);
    print("PASS native mux partial frames, flush reservation and acknowledgement fences\n");
    return 0;
}
