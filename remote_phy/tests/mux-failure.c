/* Copyright (c) 2026 bhgv
 * EN: Descriptor-free uncertain-output failure, no retransmission and retained wake storage.
 * RU: Descriptor-free uncertain-output failure, без retransmission и с retained wake storage.
 */
#include "mux/mux.h"
#include <stdio.h>
#include <stdlib.h>


/* EN/RU: Stack-owned driver state retained through explicit final endpoint release. / State до final endpoint release. */
typedef struct Driver
{
    unsigned writes; /* Only physical worker writes; inspected after join. */
    unsigned closes; /* Only physical worker/final owner disconnects. */
    unsigned destroys; /* Final retired owner destroys. */
    unsigned wakes; /* Atomic native pin wake count. */
} Driver;


/* EN/RU: Fatal release-build invariant, diagnostic borrowed. / Явный fatal invariant. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "mux failure: %s\n", why);
        exit(2);
    }
}


/* EN/RU: No incoming data; bounded nonblocking AGAIN with confirmed zero prefix. / Nonblocking AGAIN. */
static PhyTransfer reading(void *state, void *bytes, size_t count)
{
    PhyTransfer out = {0, PHY_AGAIN, PHY_PREFIX};
    (void)state;
    (void)bytes;
    (void)count;
    return out;
}


/* EN/RU: Admit two confirmed bytes and unknown tail on first physical frame; never safe to replay.
 * Подтвердить два байта с unknown tail первого physical frame; replay запрещён. */
static PhyTransfer writing(void *state, const void *bytes, size_t count)
{
    Driver *driver = state;
    PhyTransfer out = {2, PHY_ERROR, PHY_PREFIX|PHY_UNCERTAIN};
    (void)bytes;
    check(count >= 2, "first frame size");
    driver->writes++;
    return out;
}


/* EN/RU: Unexpected physical wait is a fixture failure, not an empty-poll simulation.
 * Unexpected physical wait означает failure, не empty polling. */
static int waiting(void *state, unsigned wanted, int timeout, unsigned *ready)
{
    (void)state;
    (void)wanted;
    (void)timeout;
    *ready = PHY_FAILED;
    return PHY_ERROR;
}


/* EN/RU: Atomic native wake remains available after disconnect until final destroy. / Atomic wake до destroy. */
static void waking(void *state)
{
    __atomic_add_fetch(&((Driver *)state)->wakes, 1, __ATOMIC_RELAXED);
}


/* EN/RU: Physical disconnect witness; no guest callback. / Счётчик physical disconnect без guest callback. */
static void closing(void *state)
{
    ((Driver *)state)->closes++;
}


/* EN/RU: Final ownership witness, borrowed stack state never freed by driver. / Final witness, state borrowed. */
static void destroying(void *state)
{
    ((Driver *)state)->destroys++;
}


/* EN/RU: Create a connected fake with copied callbacks/capabilities; stack state stays borrowed.
 * Создать connected fake, callbacks copied, stack state borrowed. */
static RemotePhy *carrier(Driver *driver, unsigned capabilities)
{
    static const PhyOps ops = {reading, writing, waiting, waking, NULL, NULL, closing, destroying};
    PhyInfo info = {capabilities, 0, 0, 0, 1};
    RemotePhy *phy = phycreate(&ops, driver, &info);
    check(phy != NULL, "fake carrier");
    return phy;
}


/* EN/RU: First outbound CREDIT fails uncertain; no endpoint required to trigger native pump.
 * Первый outbound CREDIT падает uncertain; endpoint не нужен для запуска pump. */
int main(void)
{
    Driver driver = {0};
    PhyMux *hub = phymuxcreate(carrier(&driver, PHY_STREAM|PHY_ORDERED|PHY_RELIABLE), 1024);
    check(hub != NULL, "native pump admission");
    /* Acquisition can race terminal worker failure; both are valid public outcomes. */
    RemotePhy *channel = phymuxchannel(hub, 0);
    if (channel)
    {
        unsigned ready;
        check(phywait(channel, PHY_READ, 5000, &ready) == PHY_OK && (ready & PHY_FAILED), "uncertain fails entire hub");
    }
    phymuxfree(hub);
    check(driver.writes == 1 && driver.closes == 1, "unknown output is never retried");
    if (channel)
    {
        phywake(channel);
        check(!driver.destroys, "live endpoint retains physical wake storage");
        phyfree(channel);
    }
    check(driver.destroys == 1, "final carrier release exactly once");
    Driver rejected = {0};
    check(!phymuxcreate(carrier(&rejected, PHY_PACKET), 1024) &&
        !rejected.writes && rejected.closes == 1 && rejected.destroys == 1, "raw packets rejected before wire effects");
    Driver invalid = {0};
    check(!phymuxcreate(carrier(&invalid, PHY_STREAM|PHY_ORDERED|PHY_RELIABLE), 1023) &&
        !invalid.writes && invalid.destroys == 1, "invalid bounds consume input without wire effects");
    puts("PASS uncertain mux output, no replay, pinned lifetime and admission cleanup");
    return 0;
}
