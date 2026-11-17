/* Copyright (c) 2026 bhgv
 * EN: Public lookup outcomes against independent raw 9P vectors, with native race/cancel callers.
 * RU: Public lookup outcomes против независимых raw 9P vectors с native race/cancel callers.
 */
#include "client.h"
#include "drivers/posix/fd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>


/* EN/RU: Explicit fatal fixture invariant, borrowed diagnostic. / Явный fatal invariant. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "lookup oracle: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Caller-owned thread context, client pinned through join; result is per operation.
 * Thread context caller-owned, client pinned до join, результат локален операции. */
typedef struct Lookup
{
    StyxClient *client; /* Borrowed attached client. */
    const char *path; /* Borrowed immutable lookup name. */
    int missing; /* Worker result copied after join. */
} Lookup;


/* EN/RU: Concurrent native caller; error outcome must survive unrelated reply/cleanup activity.
 * Concurrent native caller; результат сохраняется через чужие reply/cleanup. */
static void *lookup(void *opaque)
{
    Lookup *work = opaque;
    StyxFid *fid = styxclientlookup(work->client, work->path, NULL, &work->missing);
    check(!fid, "oracle lookup expected refusal");
    return NULL;
}


/* EN/RU: Own native client through bounded oracle, host explicitly selects error vocabulary.
 * Владеть native client, host явно выбирает error vocabulary. */
int main(int argc, char **argv)
{
    check(argc == 3, "inherited descriptor and scenario");
    const char *mode = argv[2];
    StyxClient *client = styxclientmountphy(phyposixfd(atoi(argv[1]), PHY_STREAM|PHY_ORDERED|PHY_RELIABLE),
        256, "fixture", "", 1000, 4);
    check(client != NULL, "ordinary native mount");
    int opaque = !strcmp(mode, "opaque") || !strcmp(mode, "partial");
    check(!styxclienterrorprofile(client, 99), "unknown vocabulary refused");
    if (!opaque)
    {
        check(styxclienterrorprofile(client, STYX_ERRORS_INFERNO), "explicit Inferno profile before first fid");
    }
    if (!strcmp(mode, "race"))
    {
        Lookup absent = {client, "missing", -1};
        Lookup denied = {client, "denied", -1};
        pthread_t workers[2];
        check(!pthread_create(&workers[0], NULL, lookup, &absent) &&
            !pthread_create(&workers[1], NULL, lookup, &denied), "concurrent lookups");
        check(!pthread_join(workers[0], NULL) && !pthread_join(workers[1], NULL), "lookup retirement");
        check(absent.missing == 1 && denied.missing == 0, "reply-local classification under reverse replies");
    }
    else
    {
        int missing = 77;
        StyxFid *base = NULL;
        if (!strcmp(mode, "rooted"))
        {
            base = styxclientlookup(client, ".", NULL, &missing);
            check(base && missing == 0 && !styxclienterrorprofile(client, STYX_ERRORS_OPAQUE), "captured root freezes profile");
        }
        StyxCancel *cancel = NULL;
        if (!strcmp(mode, "cancel"))
        {
            cancel = styxcancelcreate();
            check(cancel != NULL, "cancel token");
            styxcancelrequest(cancel);
        }
        const char *path = !strcmp(mode, "cloneerror") ? "." :
            !strcmp(mode, "deep") || !strcmp(mode, "partial") ? "dir/leaf" : "missing";
        StyxFid *fid = base ? styxclientlookupfrom(base, path, cancel, &missing) :
            styxclientlookup(client, path, cancel, &missing);
        int expected = !strcmp(mode, "inferno") || !strcmp(mode, "deep") || !strcmp(mode, "rooted") || !strcmp(mode, "lateerror");
        check(!fid && missing == expected, "exact per-call absence versus refusal");
        styxcancelfree(cancel);
        styxclientclunk(base);
    }
    styxclientfree(client);
    puts("PASS exact lookup outcome, conservative default and native ownership");
    return 0;
}
