/* Copyright (c) 2026 bhgv
 * EN: Actual original-core reply staging under deterministic backpressure, uncertainty and wake-pin teardown.
 * RU: Actual original-core reply staging при backpressure, uncertainty и wake-pin teardown.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "server.h"


/* EN/RU: Explicit active fixture contract, no assert or build-mode elision.
 * Явный active fixture contract без assert/build-mode зависимости. */
static void check(int valid, char *why)
{
    if (!valid)
    {
        fprint(2, "PHY server queue: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Stack driver storage outlives server and independent native wake capability.
 * Stack driver storage живёт дольше server и native wake capability. */
typedef struct State
{
    uchar request[256]; /* Owned independently encoded original Tversion. */
    unsigned length; /* Complete request extent. */
    unsigned position; /* Confirmed delivered request prefix. */
    uchar output[256]; /* Captured exact encoded reply bytes. */
    unsigned count; /* Confirmed captured prefix. */
    unsigned writes; /* Driver write admissions. */
    unsigned disconnected; /* Exactly-once disconnect. */
    unsigned destroyed; /* Exactly-once final context destruction. */
    unsigned wakes; /* Native interrupts, including after server end. */
    int blocked; /* Simulate no physical output progress. */
    int uncertain; /* Simulate unknown effects after a confirmed prefix. */
} State;


/* EN/RU: Return at most three request bytes, then AGAIN; no sockets or native wait involved.
 * Вернуть до трёх request bytes, затем AGAIN; без sockets/native wait. */
static PhyTransfer readbytes(void *state, void *bytes, size_t size)
{
    State *driver = state;
    size_t count = driver->length-driver->position;
    if (count > size)
    {
        count = size;
    }
    if (count > 3)
    {
        count = 3;
    }
    memmove(bytes, driver->request+driver->position, count);
    driver->position += count;
    PhyTransfer out = {count, count ? PHY_OK : PHY_AGAIN, PHY_PREFIX};
    return out;
}


/* EN/RU: Record exact two-byte prefixes or AGAIN/uncertain; caller bytes borrowed only through return.
 * Сохранить exact two-byte prefixes либо AGAIN/uncertain; bytes borrowed до return. */
static PhyTransfer writebytes(void *state, const void *bytes, size_t size)
{
    State *driver = state;
    driver->writes++;
    PhyTransfer out = {0, PHY_AGAIN, PHY_PREFIX};
    if (driver->blocked)
    {
        return out;
    }
    out.count = size > 2 ? 2 : size;
    out.status = PHY_OK;
    check(driver->count+out.count <= sizeof(driver->output), "capture bounds");
    memmove(driver->output+driver->count, bytes, out.count);
    driver->count += out.count;
    if (driver->uncertain)
    {
        out.flags |= PHY_UNCERTAIN;
    }
    return out;
}


/* EN/RU: Native fixture readiness; tests never block on this deterministic driver.
 * Native fixture readiness; tests не блокируются на deterministic driver. */
static int waitready(void *state, unsigned wanted, int timeout, unsigned *ready)
{
    USED(state);
    USED(timeout);
    *ready = wanted;
    return PHY_OK;
}


/* EN/RU: Independent native readiness count, no server pointer or target execution.
 * Independent native readiness count без server pointer/target execution. */
static void wake(void *state)
{
    ((State *)state)->wakes++;
}


/* EN/RU: Count wire close; native event state remains valid until destroy.
 * Считать wire close; native event state остаётся до destroy. */
static void disconnect(void *state)
{
    ((State *)state)->disconnected++;
}


/* EN/RU: Observe final native context cleanup without freeing stack storage.
 * Наблюдать final native cleanup без free stack storage. */
static void destroy(void *state)
{
    ((State *)state)->destroyed++;
}
static const PhyOps ops = {readbytes, writebytes, waitready, wake, nil, nil, disconnect, destroy}; /* Descriptor-free test driver. */


/* EN/RU: Prepare one original-codec request for reset/version, keeping expected reply independent of PHY.
 * Подготовить original-codec request reset/version, expected reply независимо от PHY. */
static void request(State *state, ushort tag)
{
    Fcall message;
    memset(&message, 0, sizeof(message));
    message.type = Tversion;
    message.tag = tag;
    message.version = "9P2000";
    message.msize = 256;
    state->length = convS2M(&message, state->request, sizeof(state->request));
    state->position = 0;
    check(state->length > 0, "request encoding");
}


/* EN/RU: Exercise exact original reply bytes, queue saturation, prefix retention and post-end wake pins.
 * Проверить original reply bytes, queue saturation, prefix retention и wake pins после end. */
int main(void)
{
    Styxserver server = {0};
    Styxops callbacks = {0};
    State state = {0};
    state.blocked = 1;
    request(&state, NOTAG);
    PhyInfo info = {PHY_STREAM|PHY_ORDERED|PHY_RELIABLE, 0, 0, 0, 1};
    RemotePhy *phy = phycreate(&ops, &state, &info);
    check(phy && !styxphyinit(&server, &callbacks, phy, 0555, 0, 1), "original PHY server init");
    StyxPhyWake *pin = styxphywakepin(&server);
    check(pin != nil && !styxwait(&server) && !styxprocess(&server), "original peer admission");
    check(!styxwait(&server) && !styxprocess(&server), "original Tversion dispatch");
    check(!styxphyspace(&server) && styxphypending(&server) > 0 && !state.count, "blocked reply was discarded");
    size_t pending = styxphypending(&server);
    check(!styxphyflush(&server, 0) && styxphypending(&server) == pending && !state.count,
        "flush timeout consumed prefix");
    state.blocked = 0;
    check(styxphyflush(&server, 0) && !styxphypending(&server) && styxphyspace(&server) == 1,
        "short output did not drain");
    uchar expected[256];
    Fcall reply;
    memset(&reply, 0, sizeof(reply));
    reply.type = Rversion;
    reply.tag = NOTAG;
    reply.version = "9P2000";
    reply.msize = 256;
    unsigned size = convS2M(&reply, expected, sizeof(expected));
    check(state.count == size && !memcmp(state.output, expected, size), "exact staged reply bytes changed");
    state.count = 0;
    state.uncertain = 1;
    request(&state, NOTAG);
    check(!styxwait(&server) && styxprocess(&server) != nil && state.count == 2, "uncertain transport accepted");
    unsigned writes = state.writes;
    check(!styxphyflush(&server, 0) && state.writes == writes && !styxphyspace(&server), "uncertain tail replayed");
    check(!styxend(&server) && !styxend(&server) && state.disconnected == 1 && !state.destroyed,
        "server retirement freed a live wake pin");
    styxphywake(pin);
    check(state.wakes == 1, "post-end native wake pin lost");
    styxphywakefree(pin);
    check(state.destroyed == 1 && state.disconnected == 1, "driver lifetime leak/double close");
    State rejected = {0};
    info.capabilities = PHY_PACKET;
    phy = phycreate(&ops, &rejected, &info);
    check(phy && styxphyinit(&server, &callbacks, phy, 0555, 0, 1) != nil &&
        rejected.destroyed == 1 && !rejected.writes && !server.priv && !server.ftab,
        "raw packet carrier admitted or leaked");
    print("PASS original PHY server queue, prefix and independent wake lifetime\n");
    return 0;
}
