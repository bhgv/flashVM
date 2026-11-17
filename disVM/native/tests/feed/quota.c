/* Copyright (c) 2026 bhgv
 * EN: Actual multiplexer admission keeps one ordinary RPC slot despite multiple watch callers.
 * RU: Настоящий multiplexer admission сохраняет ordinary RPC slot при нескольких watch callers.
 */
#include "../../client/mux_internal.h"


/* EN/RU: Native fixture checks remain active in optimized builds.
 * Native fixture checks активны в optimized builds. */
static void check(int valid, char *why)
{
    if (!valid)
    {
        fprint(2, "watch quota: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Verify watch quota refusal before effects and ordinary admission/retirement.
 * Проверить watch quota refusal до effects и ordinary admission/retirement. */
int main(void)
{
    StyxMux mux;
    memset(&mux, 0, sizeof(mux));
    StyxFlight *slots[2] = {nil, nil};
    mux.slots = slots;
    mux.limit = 2;
    mux.capacity = 256;
    mux.timeout = 1000;
    check(!pthread_mutex_init(&mux.lock, nil), "native lock");
    Fcall request;
    uchar bytes[256];
    memset(&request, 0, sizeof(request));
    request.type = Tread;
    request.fid = 2;
    request.count = 16;
    unsigned size = convS2M(&request, bytes, sizeof(bytes));
    StyxFlight *first = styxmuxsubmit(&mux, bytes, size, 256, nil, 1);
    check(first != nil && first->watching, "initial watch admission");
    check(styxmuxsubmit(&mux, bytes, size, 256, nil, 1) == nil && !mux.failed, "watch consumed final ordinary slot");
    StyxFlight *ordinary = styxmuxsubmit(&mux, bytes, size, 256, nil, 0);
    check(ordinary != nil && !ordinary->watching && ordinary->tag != first->tag, "ordinary work starved by subscription");
    pthread_mutex_lock(&mux.lock);
    styxmuxbroken(&mux);
    pthread_mutex_unlock(&mux.lock);
    StyxAnswer a = styxmuxcollect(first);
    StyxAnswer b = styxmuxcollect(ordinary);
    check(!a.attempted && !b.attempted && !a.length && !b.length, "unsent retirement invented effects");
    free(a.bytes);
    free(b.bytes);
    pthread_mutex_destroy(&mux.lock);
    print("PASS native watch quota keeps ordinary RPC capacity\n");
    return 0;
}
