/* Copyright (c) 2026 bhgv
 * EN: Original libstyx owner-thread deferred reads driven by autonomous movie-outbox readiness.
 * RU: Original libstyx owner-thread deferred reads по readiness автономного movie outbox.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "file.h"
#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>
extern int movie_test_readyfd(void); /* Borrowed under the pending control fid pin. */


/* EN/RU: Socket owner holds request refs; producer only signals native readiness.
 * Socket owner владеет request refs; producer только сигнализирует readiness. */
typedef struct Pending
{
    struct Pending *next; /* Owner-only pending chain. */
    StyxRead *request; /* Owned through deferred retirement. */
    int fd; /* Borrowed readiness, pinned by the retained fid context. */
} Pending;
static Pending *head; /* Owned by original libstyx socket thread only. */
static int readiness = -1; /* Owned epoll aggregator, registered only while requests wait. */


/* EN/RU: Fail an active fixture contract with a diagnostic, independent of build mode.
 * Проверить fixture contract с diagnostic независимо от build mode. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprint(2, "movie deferred: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Retire a pending request before its retained fid/control context is destroyed.
 * Завершить pending request до уничтожения retained fid/control context. */
static void retired(void *state)
{
    Pending *pending = state;
    for (Pending **link = &head; *link; link = &(*link)->next)
    {
        if (*link == pending)
        {
            *link = pending->next;
            break;
        }
    }
    int shared = 0;
    for (Pending *other = head; other; other = other->next)
    {
        shared |= other->fd == pending->fd;
    }
    if (!shared)
    {
        check(!epoll_ctl(readiness, EPOLL_CTL_DEL, pending->fd, nil), "remove readiness");
    }
    styxreadfree(pending->request);
    free(pending);
}


/* EN/RU: Own one live empty control Tread; returns through ordinary libstyx deferred API.
 * Retains no caller byte buffer; only original socket owner accesses requests.
 * Принять empty control Tread через libstyx deferred API, без retention caller buffer. */
void movie_test_defer(Styxserver *server, ulong *count)
{
    if (readiness < 0)
    {
        readiness = epoll_create1(EPOLL_CLOEXEC);
        check(readiness >= 0, "create readiness");
    }
    Pending *pending = calloc(1, sizeof(*pending));
    check(pending != nil, "pending allocation");
    pending->fd = movie_test_readyfd();
    check(pending->fd >= 0, "retained control readiness");
    pending->request = styxdeferread(server, pending, retired);
    check(pending->request != nil, "defer control read");
    struct epoll_event watch = {0};
    watch.events = EPOLLIN;
    watch.data.fd = pending->fd;
    check(!epoll_ctl(readiness, EPOLL_CTL_ADD, pending->fd, &watch) || errno == EEXIST, "register readiness");
    pending->next = head;
    head = pending;
    *count = 0;
    print("PENDING OFFER\n");
}


/* EN/RU: Complete ready reads on the socket owner; AGAIN retains requests, no fake EOF.
 * Завершить ready reads на socket owner; AGAIN сохраняет request, без fake EOF. */
void movie_test_complete(Styxserver *server)
{
    char bytes[MSGMAX];
    for (Pending *pending = head, *next; pending; pending = next)
    {
        next = pending->next;
        StyxTransfer result;
        styxfileread(styxreadaux(pending->request), styxreadoffset(pending->request),
            bytes, styxreadlimit(pending->request), &result);
        if (result.again)
        {
            continue;
        }
        check(styxreadreply(pending->request, bytes, result.success ? result.count : 0,
            result.success ? nil : "movie control read failed"), "deferred response");
    }
    if (readiness >= 0)
    {
        struct epoll_event ready[16];
        int count;
        do
        {
            count = epoll_wait(readiness, ready, 16, 0);
        } while (count < 0 && errno == EINTR);
        check(count >= 0, "refresh readiness");
    }
    check(!styxnotifyfd(server, head ? readiness : -1), "update owner readiness");
}


/* EN/RU: Called after libstyx teardown retires all pending requests; close native aggregator.
 * После libstyx teardown всех requests закрыть native aggregator. */
void movie_test_deferred_end(void)
{
    check(!head, "pending request survived connection retirement");
    if (readiness >= 0)
    {
        check(!close(readiness), "close readiness");
        readiness = -1;
    }
}
