/* Copyright (c) 2026 bhgv
 * EN: No-descriptor custom driver checks ownership, selector, connection, readiness and precise failures.
 * RU: Custom driver без descriptor проверяет ownership, selector, connection, readiness и precise failures.
 */
#include "phy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Explicit invariant independent of assert/build mode; diagnostic borrowed.
 * Явный invariant без assert/build-mode зависимости; diagnostic borrowed. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "remote phy: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Stack storage deliberately survives all adopted test-driver owners.
 * Stack storage намеренно живёт дольше всех test-driver owners. */
typedef struct State
{
    unsigned opens; /* Factory calls, including ambiguous-name refusal checks. */
    unsigned connects; /* Accepted native connection configurations. */
    unsigned closes; /* Actual driver disconnect calls. */
    unsigned destroys; /* Consumed driver contexts. */
    unsigned wakes; /* Native readiness signals without an OS fd. */
    size_t requested; /* Actual cap-bounded callback request. */
    int fault; /* Malformed or precise partial-result injection. */
} State;


/* EN/RU: Read native bytes with explicit fault injection; output borrowed, no blocking.
 * Читать native bytes с fault injection; output borrowed, без blocking. */
static PhyTransfer readbytes(void *state, void *bytes, size_t size)
{
    State *driver = state;
    driver->requested = size;
    memset(bytes, 'r', size);
    PhyTransfer out = {size, PHY_OK, PHY_PREFIX};
    if (driver->fault == 1)
    {
        out.count = size+1;
    }
    else if (driver->fault == 2)
    {
        out.count = 0;
        out.status = PHY_AGAIN;
    }
    else if (driver->fault == 3)
    {
        out.count = 0;
        out.status = PHY_EOF;
    }
    return out;
}


/* EN/RU: Return precise native partial/error outcomes without changing borrowed input.
 * Вернуть precise native partial/error results, не меняя borrowed input. */
static PhyTransfer writebytes(void *state, const void *bytes, size_t size)
{
    State *driver = state;
    (void)bytes;
    driver->requested = size;
    PhyTransfer out = {size, PHY_OK, PHY_PREFIX};
    if (driver->fault == 4)
    {
        out.flags |= PHY_UNCERTAIN;
    }
    else if (driver->fault == 5)
    {
        out.status = PHY_AGAIN;
    }
    else if (driver->fault == 6)
    {
        out.count = 0;
    }
    return out;
}


/* EN/RU: Simulated driver-local event flag, not a POSIX handle or wire message.
 * Simulated driver-local event flag, не POSIX handle/wire message. */
static int waitready(void *state, unsigned wanted, int timeout, unsigned *ready)
{
    State *driver = state;
    (void)timeout;
    *ready = driver->wakes ? PHY_WAKE : wanted;
    driver->wakes = 0;
    return PHY_OK;
}


/* EN/RU: Signal driver-local native event; no transport I/O or guest callback.
 * Сигнал driver-local native event без transport I/O/guest callback. */
static void wake(void *state)
{
    ((State *)state)->wakes++;
}


/* EN/RU: Available-byte hint independent of permissions/EOF; output borrowed.
 * Available-byte hint независимо от permissions/EOF; output borrowed. */
static int available(void *state, size_t *bytes)
{
    (void)state;
    *bytes = 17;
    return PHY_OK;
}


/* EN/RU: Accept exactly this fixture's native bus configuration; parameters borrowed, OK/ERROR.
 * Принять только native bus config fixture; parameters borrowed, OK/ERROR. */
static int connect(void *state, const void *parameters, size_t size)
{
    if (size != sizeof(unsigned) || !parameters || *(const unsigned *)parameters != 42)
    {
        return PHY_ERROR;
    }
    ((State *)state)->connects++;
    return PHY_OK;
}


/* EN/RU: Count actual disconnect; stack context remains alive until final destroy.
 * Считать actual disconnect; stack context остаётся до final destroy. */
static void disconnect(void *state)
{
    ((State *)state)->closes++;
}


/* EN/RU: Count final context disposal, without freeing caller-owned stack storage.
 * Считать final context disposal без free caller-owned stack storage. */
static void destroy(void *state)
{
    ((State *)state)->destroys++;
}
static const PhyOps ops = {readbytes, writebytes, waitready, wake, available, connect, disconnect, destroy}; /* No OS methods. */


/* EN/RU: Host-selected native factory adopts stack context only on successful wrapper creation.
 * Host-selected native factory принимает context только при successful wrapper creation. */
static RemotePhy *openfake(const void *parameters, size_t size)
{
    if (size != sizeof(State) || !parameters)
    {
        return NULL;
    }
    State *state = (State *)parameters;
    state->opens++;
    PhyInfo info = {PHY_STREAM|PHY_ORDERED|PHY_RELIABLE|PHY_HALF_DUPLEX, 8, 3, 2, 0};
    return phycreate(&ops, state, &info);
}


/* EN/RU: Verify carrier-neutral behavior without sockets, descriptors or 9P codecs.
 * Проверить carrier-neutral behavior без sockets/descriptors/9P codecs. */
int main(void)
{
    State state = {0};
    PhyDriver drivers[2] = {{"custom", openfake}, {"custom", openfake}};
    check(!physelect(drivers, 2, "custom", &state, sizeof(state)) && !state.opens, "ambiguous selector called factory");
    check(!physelect(drivers, 1, "unknown", &state, sizeof(state)), "unknown carrier fallback");
    RemotePhy *phy = physelect(drivers, 1, "custom", &state, sizeof(state));
    check(phy != NULL && state.opens == 1, "selected factory");
    char bytes[8];
    PhyTransfer out = phyread(phy, bytes, sizeof(bytes));
    check(out.status == PHY_ERROR && !state.requested, "disconnected driver entered");
    unsigned config = 41;
    check(phyconnect(phy, &config, sizeof(config)) == PHY_ERROR && !state.connects, "bad config accepted");
    config = 42;
    check(phyconnect(phy, &config, sizeof(config)) == PHY_OK && state.connects == 1, "native connect");
    check(phyconnect(phy, NULL, 0) == PHY_OK && phyconnect(phy, &config, sizeof(config)) == PHY_ERROR &&
        state.connects == 1, "live carrier silently reconfigured");
    PhyInfo info;
    check(phyinfo(phy, &info) && info.connected && info.mtu == 8, "local capabilities");
    out = phyread(phy, bytes, sizeof(bytes));
    check(out.status == PHY_OK && out.count == 3 && state.requested == 3 && bytes[0] == 'r', "bounded read");
    out = phywrite(phy, bytes, sizeof(bytes));
    check(out.status == PHY_OK && out.count == 2 && state.requested == 2, "bounded write");
    state.requested = 0;
    check(phyread(phy, NULL, 0).status == PHY_OK && !state.requested, "zero read entered driver");
    size_t count = 999;
    check(phyavailable(phy, &count) == PHY_OK && count == 17, "available hint");
    phywake(phy);
    unsigned ready;
    check(phywait(phy, PHY_READ, -1, &ready) == PHY_OK && ready == PHY_WAKE, "sticky native wake");
    state.fault = 1;
    out = phyread(phy, bytes, sizeof(bytes));
    check(out.status == PHY_ERROR && !out.count && (out.flags & PHY_UNCERTAIN), "overflow acknowledgement trusted");
    state.fault = 2;
    check(phyread(phy, bytes, sizeof(bytes)).status == PHY_AGAIN, "AGAIN became EOF");
    state.fault = 3;
    check(phyread(phy, bytes, sizeof(bytes)).status == PHY_EOF, "explicit EOF lost");
    state.fault = 4;
    out = phywrite(phy, bytes, sizeof(bytes));
    check(out.count == 2 && (out.flags & PHY_UNCERTAIN), "uncertain prefix lost");
    state.fault = 5;
    out = phywrite(phy, bytes, sizeof(bytes));
    check(out.status == PHY_AGAIN && out.count == 2, "partial AGAIN lost");
    state.fault = 6;
    out = phywrite(phy, bytes, sizeof(bytes));
    check(out.status == PHY_ERROR && (out.flags & PHY_UNCERTAIN), "zero progress accepted");
    phydisconnect(phy);
    phydisconnect(phy);
    check(state.closes == 1, "double disconnect");
    phywake(phy);
    phyfree(phy);
    check(state.destroys == 1 && state.closes == 1, "driver lifetime");
    puts("PASS descriptor-free PHY selector, configuration, readiness and precise ownership");
    return 0;
}
