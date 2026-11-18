/* Copyright (c) 2026 bhgv
 * EN: Original-Styx acceptance of automatic named negotiation, isolated clients and bidirectional selected renewal.
 * RU: Приёмочные тесты автоматического согласования, изоляции клиентов и двусторонних сбросов через исходный Styx.
 */
#include "../internal.h"
#include "mount.h"
#include <stdio.h>


/* EN/RU: Original server/client carrier fixture, owned client/NULL; physical owner lives through join.
 * Исходная carrier-fixture сервера/клиента, свой client/NULL; physical owner существует до join.
 */
extern StyxClient *linkfixture(void);


/* EN/RU: Break actual physical carrier, no synthetic broker notification.
 * Разорвать настоящий physical carrier, без искусственного уведомления брокера.
 */
extern void linkfixturebreak(void);


/* EN/RU: Join original server and release physical pumps after every client/control owner retires.
 * Завершить исходный сервер и освободить pumps после всех client/control owners.
 */
extern void linkfixturefree(void);


/* EN/RU: Fatal explicit test contract with source line, never optimized away.
 * Явная обязательная проверка с номером строки, не удаляется оптимизатором.
 */
static void require(int condition, int line)
{
    if (!condition)
    {
        fprintf(stderr, "ordered broker case %d line %d\n", ORDERED_CASE, line);
        exit(2);
    }
}
#define CHECK(value) require((value) != 0, __LINE__) /* Explicit evaluated diagnostic. */


/* EN/RU: Wait and copy exactly one client payload/ordinal, caller keeps peer pinned.
 * Дождаться и скопировать одну запись/номер клиента, caller удерживает peer.
 */
static void take(StyxOrderedClient *client, unsigned long long expected, const char *text)
{
    char bytes[128]; /* Private output copy. */
    size_t size; /* Exact returned body extent. */
    unsigned long long ordinal; /* Exact full unsigned number. */
    CHECK(styxorderedclientwait(client, 3000));
    CHECK(styxorderedclientnext(client, &ordinal, bytes, sizeof(bytes), &size) == STYX_ORDER_RECORD);
    CHECK(ordinal == expected && size == strlen(text) && !memcmp(bytes, text, size));
}


/* EN/RU: Test the complete original control path; MAX seeding is fixture-only on both agreed endpoints.
 * Проверить полный исходный control path; MAX-seed применяется только в fixture с согласованными концами.
 */
int main(void)
{
    StyxClient *client = linkfixture(); /* Owned original native connection. */
    CHECK(client && styxclientretain(client));
    DVM_Provider *grant = styxmountproviderat(client, ".", FVM_ACCESS_MASK); /* Consumes retained connection pin. */
    DVM_TerminalChannels channels = {0}; /* Captured native root, no ambient path fallback. */
    channels.provider = grant;
    strcpy(channels.receive, "records");
    strcpy(channels.send, "records");
    unsigned long tags[] = {0, 7, 9, 99}; /* Distinct predeclared application inbox capabilities. */
    StyxBidirControl *control = styxbidircontrol(client, &channels, 512, 1); /* Owned ordinary S-record workers. */
    StyxOrdered *peer = styxbidirordered(control, 128, ORDERED_CASE == 5 ? 8 : 16, tags, 4); /* Consumes control. */
    CHECK(peer);
    StyxOrderedClient *a = styxorderedclient(peer, 7); /* Fixed tag7 only. */
    StyxOrderedClient *b = styxorderedclient(peer, 9); /* Fixed tag9 only. */
    StyxOrderedClient *zero = styxorderedclient(peer, 0); /* Inner zero, never transport control. */
    CHECK(a && b && zero && !styxorderedclient(peer, 8));
    CHECK(!styxorderedclientsend(a, "unnegotiated", 12));
    if (ORDERED_CASE == 5)
    {
        CHECK(styxorderstart(peer->engine, 7, STYX_ORDER_RX, ~0ULL - 1));
        CHECK(styxorderstart(peer->engine, 7, STYX_ORDER_TX, ~0ULL - 1));
    }
    CHECK(styxorderedoffer(peer, 1, 71));
    CHECK(!styxorderedoffer(peer, 1, 71));
    if (ORDERED_CASE == 2 || ORDERED_CASE == 3)
    {
        CHECK(!styxorderedwait(peer, STYX_ORDER_TX, 3000));
        CHECK(styxorderedstate(peer) == STYX_CONTROL_PROTOCOL);
    }
    else
    {
        if (ORDERED_CASE == 1)
        {
            CHECK(styxorderedofferwait(peer, 1, 3000));
            CHECK(!styxorderedready(peer, STYX_ORDER_TX));
            CHECK(styxorderedoffer(peer, 2, 71));
        }
        CHECK(styxorderedwait(peer, STYX_ORDER_TX, 3000));
        CHECK(styxorderedwait(peer, STYX_ORDER_RX, 3000));
        take(b, 0, "nine");
        if (ORDERED_CASE == 4)
        {
            take(b, 1, "rx-live"); /* FIFO barrier: all old tag7 records were accepted before physical loss. */
            linkfixturebreak();
            CHECK(!styxorderedclientwait(styxorderedclient(peer, 99), 3000));
            CHECK(styxorderedstate(peer) != STYX_CONTROL_LIVE);
            take(a, 0, "zero"); /* Already-ready data survives the actual carrier loss. */
        }
        else
        {
            char oversized[96] = {0}; /* Remote accepted body96 includes encoded tag. */
            CHECK(!styxorderedclientsend(a, oversized, sizeof(oversized)));
            CHECK(styxorderedclientsend(a, "out0", 4));
            if (ORDERED_CASE == 5)
            {
                CHECK(styxorderedclientsend(a, "outmax", 6)); /* MAX automatically queues selected renewal. */
            }
            else
            {
                CHECK(styxorderedrenew(peer, 7));
            }
            CHECK(!styxorderedclientsend(a, "held", 4));
            CHECK(styxorderedclientsend(b, "unrelated", 9));
            CHECK(styxorderedclientsend(zero, "local-out", 9));
            take(b, 1, "rx-live"); /* Peer renewal is pending, tag7 old records still unread. */
            take(b, 2, "tx-ready"); /* Matching ACK already dispatched, independent RX renewal remains pending. */
            CHECK(styxorderedclientsend(a, "outnew", 6));
            take(b, 3, "verified"); /* Original peer verified TX restarted at zero and local0 remained nested. */
            take(zero, 0, "local-in");
            char tiny = 'x'; /* SMALL must not acknowledge the last old boundary. */
            unsigned long long ordinal = 123; /* Failure sentinel. */
            size_t size = 456; /* Required payload extent. */
            CHECK(styxorderedclientnext(a, &ordinal, &tiny, 1, &size) == STYX_ORDER_SMALL);
            CHECK(ordinal == 123 && tiny == 'x');
            pthread_mutex_lock(&peer->lock);
            CHECK(a->rx_pending && !a->tx_pending);
            pthread_mutex_unlock(&peer->lock);
            if (ORDERED_CASE != 5)
            {
                take(a, 0, "zero");
            }
            take(a, ORDERED_CASE == 5 ? ~0ULL - 1 : 1, "one");
            take(a, ORDERED_CASE == 5 ? ~0ULL : 2, "two");
            take(a, 0, "new"); /* Automatic ACK after the final old copy, no guest pause or callback. */
            CHECK(styxorderedservicewait(peer, 3000));
            char service[16]; /* Native unknown-service inbox, inaccessible through client0. */
            CHECK(styxorderedservicenext(peer, service, 1, &size) == STYX_CONTROL_SMALL && size == 6);
            CHECK(styxorderedservicenext(peer, service, sizeof(service), &size) == STYX_CONTROL_RECORD && !memcmp(service, "native", 6));
            CHECK(!styxorderedstate(peer));
        }
    }
    styxorderedfree(peer);
    dvm_provider_release(grant);
    styxclientshutdown(client);
    styxclientfree(client);
    linkfixturefree();
    return 0;
}
