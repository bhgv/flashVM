/* Copyright (c) 2026 bhgv
 * EN: Independent mux acceptance: credit isolation, bounded queues, binary wraparound and pinned stop.
 * RU: Независимые проверки mux: credit isolation, bounded queues, binary wraparound и pinned stop.
 */
#define _POSIX_C_SOURCE 200809L
#include "mux/bidir.h"
#include "drivers/posix/fd.h"
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/* EN/RU: Release-build invariant, borrowed diagnostic; terminate on failure. / Явный invariant, fatal failure. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "mux fixture: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Monotonic milliseconds for fixture-wide finite I/O budgets. / Monotonic ms для bounded fixture I/O. */
static long long now(void)
{
    struct timespec value;
    check(!clock_gettime(CLOCK_MONOTONIC, &value), "monotonic clock");
    return (long long)value.tv_sec*1000+value.tv_nsec/1000000;
}


/* EN/RU: Transfer exact bytes through real logical API under one deadline; buffers borrowed.
 * Передать exact bytes через logical API под одним deadline; buffers borrowed. */
static void transfer(RemotePhy *phy, unsigned char *bytes, size_t size, int writing)
{
    long long deadline = now()+5000;
    size_t offset = 0;
    while (offset < size)
    {
        PhyTransfer out = writing ? phywrite(phy, bytes+offset, size-offset) : phyread(phy, bytes+offset, size-offset);
        check((out.status == PHY_OK || out.status == PHY_AGAIN) && !(out.flags & PHY_UNCERTAIN), "exact transfer status");
        offset += out.count;
        if (!out.count)
        {
            unsigned ready;
            int remaining = (int)(deadline-now());
            check(remaining > 0 && phywait(phy, writing ? PHY_WRITE : PHY_READ, remaining, &ready) == PHY_OK && ready &&
                !(ready & PHY_FAILED), "transfer deadline/readiness");
        }
    }
}


/* EN/RU: Native producer owns borrowed channel/buffer through join. / Native producer, borrowed channel/buffer до join. */
typedef struct Work
{
    RemotePhy *phy; /* Caller-retained channel. */
    unsigned char *bytes; /* Caller-retained immutable bytes. */
    size_t size; /* Exact transfer size. */
} Work;


/* EN/RU: Write one large stream independently of native reader. / Записать large stream независимо от reader. */
static void *writing(void *state)
{
    Work *work = state;
    transfer(work->phy, work->bytes, work->size, 1);
    return NULL;
}


/* EN/RU: Exercise two channels over one pair; no peer metadata or internal mux structures used.
 * Проверить два канала через одну пару, без internal mux structures. */
static void streams(void)
{
    int descriptors[2];
    check(!socketpair(AF_UNIX, SOCK_STREAM, 0, descriptors), "native carrier pair");
    PhyBidir *a = phybidircreate(phyposixfd(descriptors[0], PHY_STREAM|PHY_ORDERED|PHY_RELIABLE), 1024);
    PhyBidir *b = phybidircreate(phyposixfd(descriptors[1], PHY_STREAM|PHY_ORDERED|PHY_RELIABLE), 2048);
    check(a && b, "asymmetric capacity muxes");
    RemotePhy *a0 = phybidirchannel(a, 0);
    RemotePhy *a1 = phybidirchannel(a, 1);
    RemotePhy *b0 = phybidirchannel(b, 0);
    RemotePhy *b1 = phybidirchannel(b, 1);
    check(a0 && a1 && b0 && b1 && !phybidirchannel(a, 2) && !phybidirchannel(a, 0), "channel ownership");
    unsigned char source[65537];
    unsigned char result[65537];
    for (size_t i = 0; i < sizeof(source); i++)
    {
        source[i] = (unsigned char)(i*37+i/251);
    }
    transfer(a0, source, 2048, 1);
    unsigned ready;
    size_t available = 0;
    long long deadline = now()+5000;
    while (available < 2048)
    {
        check(now() < deadline && phywait(b0, PHY_READ, 100, &ready) == PHY_OK &&
            phyavailable(b0, &available) == PHY_OK, "receive credit fills exact peer bound");
    }
    transfer(a0, source, 1024, 1);
    PhyTransfer out = phywrite(a0, source, 1);
    check(out.status == PHY_AGAIN && !out.count, "exhausted credit retains bounded TX");
    transfer(a1, source, 8, 1);
    transfer(b1, result, 8, 0);
    check(!memcmp(source, result, 8), "full channel0 does not block channel1");
    transfer(b1, source+8, 8, 1);
    transfer(a1, result, 8, 0);
    check(!memcmp(source+8, result, 8), "reverse RPC while other queue full");
    transfer(b0, result, 2048, 0);
    check(!memcmp(source, result, 2048), "bounded queued prefix");
    transfer(b0, result, 1024, 0);
    check(!memcmp(source, result, 1024), "credit replenishment");
    Work forward = {a0, source, sizeof(source)};
    Work reverse = {b1, source, sizeof(source)};
    pthread_t writers[2];
    check(!pthread_create(&writers[0], NULL, writing, &forward) &&
        !pthread_create(&writers[1], NULL, writing, &reverse), "parallel stream producers");
    transfer(b0, result, sizeof(result), 0);
    check(!memcmp(source, result, sizeof(source)), "forward ring wrap and binary bytes");
    transfer(a1, result, sizeof(result), 0);
    check(!memcmp(source, result, sizeof(source)), "reverse ring wrap and binary bytes");
    check(!pthread_join(writers[0], NULL) && !pthread_join(writers[1], NULL), "native stream producer retirement");
    phywake(a1);
    check(phywait(a1, PHY_READ, 0, &ready) == PHY_OK && (ready & PHY_WAKE), "sticky early local wake");
    phyfree(a0);
    check(phywait(b0, PHY_READ, 5000, &ready) == PHY_OK && (ready & PHY_HANGUP), "logical close notification");
    out = phyread(b0, result, 1);
    check(out.status == PHY_EOF && !out.count, "logical close preserves stream EOF");
    transfer(a1, source, 8, 1);
    transfer(b1, result, 8, 0);
    check(!memcmp(source, result, 8), "other channel survives close");
    phybidirfree(a);
    phywake(a1);
    check(phywait(a1, PHY_READ, 0, &ready) == PHY_OK && (ready & PHY_FAILED) && (ready & PHY_WAKE), "late pin after pump join");
    phyfree(a1);
    phybidirfree(b);
    phyfree(b0);
    phyfree(b1);
}


/* EN/RU: Reject crafted framing/credit abuse with an independent raw peer; no production encoder.
 * Отклонить crafted frame/credit abuse от raw peer, без production encoder. */
static void malformed(unsigned kind)
{
    int descriptors[2];
    check(!socketpair(AF_UNIX, SOCK_STREAM, 0, descriptors), "oracle pair");
    PhyBidir *hub = phybidircreate(phyposixfd(descriptors[0], PHY_STREAM|PHY_ORDERED|PHY_RELIABLE), 1024);
    RemotePhy *channel = phybidirchannel(hub, 0);
    check(hub && channel, "oracle channel");
    unsigned char bytes[32] = {'P', '9', 'M', 'X', 1, 2, 0, 0, 4, 0, 0, 0, 0, 4, 0, 0};
    size_t count = 16;
    switch (kind)
    {
    case 0:
        bytes[0] = 'X';
        break;
    case 1:
        bytes[4] = 2;
        break;
    case 2:
        bytes[6] = 2;
        break;
    case 3:
        bytes[7] = 1;
        break;
    case 4:
        bytes[8] = 255;
        bytes[9] = 255;
        break;
    case 5:
        bytes[12] = bytes[13] = 0;
        break;
    case 6:
        bytes[14] = 32;
        break;
    case 7:
        bytes[5] = 1;
        break;
    case 8:
        bytes[5] = 0;
        bytes[8] = 0;
        count = 12;
        break;
    case 9:
        bytes[5] = 3;
        break;
    case 10:
        bytes[8] = 3;
        break;
    case 11:
        bytes[5] = 1;
        bytes[6] = 1;
        bytes[8] = 0;
        memcpy(bytes+12, bytes, 12);
        count = 24;
        break;
    case 12:
        bytes[12] = 0;
        bytes[13] = 0;
        bytes[14] = 16;
        memcpy(bytes+16, bytes, 16);
        count = 32;
        break;
    default:
        check(0, "known oracle variant");
    }
    check(write(descriptors[1], bytes, count) == (ssize_t)count, "raw oracle write");
    unsigned ready;
    check(phywait(channel, PHY_READ, 5000, &ready) == PHY_OK && (ready & PHY_FAILED), "malformed mux fails closed");
    phybidirfree(hub);
    phywake(channel);
    phyfree(channel);
    close(descriptors[1]);
}


/* EN/RU: Run isolated positive and independent malformed-input acceptance. / Выполнить positive и negative acceptance. */
int main(void)
{
    streams();
    for (unsigned kind = 0; kind < 13; kind++)
    {
        malformed(kind);
    }
    puts("PASS mux credits, isolation, wraparound, close, retirement and raw oracle rejection");
    return 0;
}
