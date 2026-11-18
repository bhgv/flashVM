/* Copyright (c) 2026 bhgv
 * EN: Announced resource fids resolve inside the route's actual directory/file grant, never at client attach root.
 * RU: Announced resource fids разрешаются внутри directory/file grant route, не в attach root клиента.
 */
#include "route/types.h"
#include "resource/announced.h"
#include "announcement.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Atomic ambient-service open sentinel from the actual native resource fixture backend.
 * Atomic sentinel открытия ambient service из настоящего native fixture backend.
 */
extern unsigned terminalgrantambientresources(void);


/* EN/RU: Explicit native fixture contract; why borrowed until return/exit, no optimized-away checks.
 * Явный native fixture contract, why borrowed до return/exit, без optimized-away checks.
 */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "terminal resource grant: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Native synchronous sink checks independent binary oracle, increments caller-owned invocation count.
 * Native synchronous sink проверяет independent binary oracle и считает caller-owned invocations.
 */
static FVM_Result staging(const void *bytes, size_t size, void *state)
{
    static const unsigned char expected[] = {0, 128, 255, 0, 'x'}; /* Actual prepared VFS resource content. */
    unsigned *calls = state; /* Caller-owned until synchronous fetch returns. */
    check(size == sizeof(expected) && !memcmp(bytes, expected, size), "actual scoped resource service content");
    (*calls)++;
    return FVM_OK;
}


/* EN/RU: Consume one general item; receiver alone consumes typed offers, all payload copies stay caller-owned.
 * Потребить одну general item; receiver один потребляет typed offers, copies принадлежат caller.
 */
static void general(StyxBidirRoute *route, unsigned long expected)
{
    unsigned char bytes[256]; /* One bounded copied record. */
    unsigned long tag; /* Exact numeric tag. */
    size_t size; /* Exact body size. */
    check(styxbidirroutewait(route, 0, 5000) &&
        styxbidirroutenext(route, &tag, bytes, sizeof(bytes), &size) == STYX_CONTROL_RECORD && tag == expected,
        "independent general/control declaration");
}


/* EN/RU: Borrow grant/client through setup; own route/receiver through every fetch, then free in borrower order.
 * Borrow grant/client до setup, владеть route/receiver до конца fetch, free в порядке завершения borrowers.
 */
void terminalgrantresource(StyxClient *client, const DVM_TerminalChannels *channels, int file_root)
{
    StyxRouteType type = {MOVIE_OFFER_TYPE, MOVIE_OFFER_VERSION}; /* Exclusive original announcement schema. */
    StyxBidirRoute *route = styxbidirbindingroutes(client, channels, 512, 2, NULL, 0, &type, 1);
    StyxBidirMovieReceiver *receiver = route ? styxbidirmoviereceiver(route, 0, 5000) : NULL; /* Exact client, rooted route. */
    check(route && receiver, "rooted resource route and receiver");
    general(route, 41);
    general(route, 0);
    check(styxbidirmoviereceiverwait(receiver, 1, 5000) == 1, "original READY offer received automatically");
    char origin[64] = "unchanged"; /* Caller-visible final origin sentinel. */
    unsigned calls = 0; /* Actual synchronous sink count. */
    FVM_Result result = styxbidirannouncedfetch(FVM_DESKTOP_FETCH_CODE, "/ok.swf", "/base.swf", staging,
        &calls, origin, sizeof(origin), receiver);
    check(!terminalgrantambientresources(), "announced path never probes ambient movie generation");
    if (file_root)
    {
        check(result != FVM_OK && !calls && !strcmp(origin, "unchanged"),
            "file-only control grant cannot acquire a child service outside its root");
    }
    else
    {
        check(result == FVM_OK && calls == 1 && !strcmp(origin, "/ok.swf"), "resource fid walks inside the same captured job grant");
    }
    check(styxbidirmoviereceiverfree(receiver), "only own offer worker joined after fetch callers retire");
    styxbidirroutefree(route);
    check(styxclientalive(client), "ordinary client survives scoped resource retirement");
}
