/* Copyright (c) 2026 bhgv
 * EN: Exercise the real custom memory driver's prefix, backpressure, wake and lifetime contracts.
 * RU: Проверить prefix/backpressure/wake/lifetime контракты настоящего memory driver.
 */
#define _POSIX_C_SOURCE 200809L
#include "memory.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/* EN/RU: One I/O owner waits; another native thread may only signal phywake.
 * Один I/O owner ждёт; другой native thread может только вызвать phywake.
 */
typedef struct WakeTest
{
    RemotePhy *phy;          /* Main pins endpoint through join; worker owns all wait calls. */
    pthread_mutex_t lock;    /* Test-only admission/completion observations, not a PHY implementation lock. */
    pthread_cond_t changed;  /* No scheduler sleeps are used to guess wait admission. */
    unsigned admitted;       /* Highest announced wait round. */
    unsigned completed;      /* Highest completed wait round. */
    int failed;              /* Published under test lock; breaks main's observation waits. */
} WakeTest;


/* EN: state borrows WakeTest until join; perform 500 waits. Announce before wait,
 * permitting deliberate pre-entry wake races. Return NULL; failed carries error.
 * RU: state заимствует WakeTest до join; выполнить 500 waits. Объявлять до wait,
 * разрешая pre-entry wake races. Вернуть NULL; ошибка в failed.
 */
static void *waiting(void *state)
{
    WakeTest *test = state;
    for (unsigned round = 1; round <= 500; round++)
    {
        pthread_mutex_lock(&test->lock);
        test->admitted = round;
        pthread_cond_signal(&test->changed);
        pthread_mutex_unlock(&test->lock);
        unsigned ready = 0;
        int good = phywait(test->phy, PHY_READ, -1, &ready) == PHY_OK && (ready & PHY_WAKE);
        pthread_mutex_lock(&test->lock);
        test->completed = round;
        test->failed = !good;
        pthread_cond_signal(&test->changed);
        pthread_mutex_unlock(&test->lock);
        if (!good)
        {
            break;
        }
    }
    return NULL;
}


/* EN: Borrow connected phy; drive native wait/wake round trips and join before returning.
 * Return 1/0. Startup failure releases test synchronization; impossible join failure terminates.
 * RU: Заимствовать connected phy; выполнить native wait/wake rounds и join до возврата.
 * Вернуть 1/0. При startup failure освободить синхронизацию; при failed join завершить процесс.
 */
static int concurrent_wake(RemotePhy *phy)
{
    WakeTest test = {0};
    test.phy = phy;
    if (pthread_mutex_init(&test.lock, NULL))
    {
        return 0;
    }
    if (pthread_cond_init(&test.changed, NULL))
    {
        pthread_mutex_destroy(&test.lock);
        return 0;
    }
    pthread_t thread;
    if (pthread_create(&thread, NULL, waiting, &test))
    {
        pthread_cond_destroy(&test.changed);
        pthread_mutex_destroy(&test.lock);
        return 0;
    }
    for (unsigned round = 1; round <= 500; round++)
    {
        pthread_mutex_lock(&test.lock);
        while (test.admitted < round && !test.failed)
        {
            pthread_cond_wait(&test.changed, &test.lock);
        }
        int failed = test.failed;
        pthread_mutex_unlock(&test.lock);
        if (failed)
        {
            break;
        }
        phywake(phy); /* The only cross-owner API; no lifecycle/read/write access races the waiter. */
        pthread_mutex_lock(&test.lock);
        while (test.completed < round && !test.failed)
        {
            pthread_cond_wait(&test.changed, &test.lock);
        }
        failed = test.failed;
        pthread_mutex_unlock(&test.lock);
        if (failed)
        {
            break;
        }
    }
    if (pthread_join(thread, NULL))
    {
        _Exit(2); /* Do not unwind borrowed test/endpoint storage under a live thread. */
    }
    int good = !test.failed && test.completed == 500;
    good = pthread_cond_destroy(&test.changed) == 0 && good;
    good = pthread_mutex_destroy(&test.lock) == 0 && good;
    return good;
}


/* EN: Fill/drain a bounded connected pair, then disconnect with a readable prefix.
 * No arguments. Return 1 only if every invariant holds; free both endpoints on all outcomes.
 * RU: Заполнить/опустошить bounded pair, затем disconnect с читаемым prefix.
 * Аргументов нет. Вернуть 1 при всех invariants; при любом исходе освободить оба endpoints.
 */
static int driver_contract(void)
{
    MemoryStats stats = {0};
    RemotePhy *left = NULL;
    RemotePhy *right = NULL;
    if (!memory_pair(&left, &right, &stats, 0))
    {
        return 0;
    }
    int good = 1;
    unsigned ready = 99;
    phywake(right); /* Notification before wait must remain sticky, without putting data on the wire. */
    good = phywait(right, PHY_READ, -1, &ready) == PHY_OK && ready == PHY_WAKE;
    good = good && phywait(right, PHY_READ, 0, &ready) == PHY_OK && ready == 0;
    size_t hint = 123;
    good = good && phyavailable(right, &hint) == PHY_UNSUPPORTED && hint == 123;
    PhyTransfer out = phyread(right, NULL, 0);
    good = good && out.status == PHY_OK && out.count == 0 && stats.reads == 0;
    unsigned char source[MEMORY_CAPACITY];
    for (unsigned index = 0; index < MEMORY_CAPACITY; index++)
    {
        source[index] = (unsigned char)index;
    }
    size_t sent = 0;
    while (good && sent < sizeof(source))
    {
        out = phywrite(left, source + sent, sizeof(source) - sent);
        good = out.status == PHY_OK && out.count > 0 && out.count <= 2 && out.flags == PHY_PREFIX;
        sent += out.count;
    }
    out = phywrite(left, source, 1);
    good = good && out.status == PHY_AGAIN && out.count == 0 && !(out.flags & PHY_UNCERTAIN);
    good = good && phywait(left, PHY_WRITE, 0, &ready) == PHY_OK && !(ready & PHY_WRITE);
    phyfree(left); /* Its accepted prefix remains readable through the peer's remaining shared reference. */
    left = NULL;
    good = good && stats.destroyed == 1 && phywait(right, PHY_READ, 0, &ready) == PHY_OK &&
        (ready & (PHY_READ | PHY_HANGUP)) == (PHY_READ | PHY_HANGUP);
    unsigned char received[MEMORY_CAPACITY];
    size_t count = 0;
    while (good && count < sizeof(received))
    {
        out = phyread(right, received + count, sizeof(received) - count);
        good = out.status == PHY_OK && out.count > 0 && out.count <= 3;
        count += out.count;
    }
    out = phyread(right, received, 1);
    good = good && out.status == PHY_EOF && out.count == 0 && !memcmp(source, received, sizeof(source));
    phyfree(right);
    return good && stats.destroyed == 2;
}


/* EN: Test a precise accepted prefix followed by uncertainty; do not retry after error.
 * Return 1/0 and always retire the two owned endpoints before observations leave scope.
 * RU: Проверить точный принятый prefix с uncertainty; не повторять после ошибки.
 * Вернуть 1/0; всегда освободить endpoints до выхода observations из scope.
 */
static int uncertain_prefix(void)
{
    MemoryStats stats = {0};
    RemotePhy *left = NULL;
    RemotePhy *right = NULL;
    if (!memory_pair(&left, &right, &stats, 5))
    {
        return 0;
    }
    const char text[] = "abcdef";
    size_t sent = 0;
    PhyTransfer out = {0, PHY_OK, PHY_PREFIX};
    for (unsigned attempt = 0; attempt < 3; attempt++)
    {
        out = phywrite(left, text + sent, sizeof(text) - 1 - sent);
        sent += out.count;
        if (out.status == PHY_ERROR)
        {
            break;
        }
    }
    int good = sent == 5 && out.status == PHY_ERROR && out.count == 1 &&
        out.flags == (PHY_PREFIX | PHY_UNCERTAIN) && stats.writes == 3;
    char received[5];
    size_t count = 0;
    while (good && count < sizeof(received))
    {
        out = phyread(right, received + count, sizeof(received) - count);
        good = out.status == PHY_OK && out.count > 0;
        count += out.count;
    }
    good = good && !memcmp(received, text, 5);
    phyfree(left);
    phyfree(right);
    return good && stats.destroyed == 2;
}


/* EN: No arguments. Verify finite monotonic wait and repeated concurrent wake on an empty pair.
 * Return 1/0; deadline measures absence of data, not a claim of realtime scheduler guarantees.
 * RU: Аргументов нет. Проверить finite monotonic wait и concurrent wake на пустой pair.
 * Вернуть 1/0; измеряется отсутствие данных, не гарантии realtime scheduler.
 */
static int wait_contract(void)
{
    MemoryStats stats = {0};
    RemotePhy *left = NULL;
    RemotePhy *right = NULL;
    if (!memory_pair(&left, &right, &stats, 0))
    {
        return 0;
    }
    struct timespec before;
    struct timespec after;
    unsigned ready = 99;
    int good = !clock_gettime(CLOCK_MONOTONIC, &before) && phywait(right, PHY_READ, 20, &ready) == PHY_OK &&
        !clock_gettime(CLOCK_MONOTONIC, &after) && ready == 0;
    if (good)
    {
        long long elapsed = (after.tv_sec - before.tv_sec) * 1000000000LL + after.tv_nsec - before.tv_nsec;
        good = elapsed >= 10000000LL && concurrent_wake(right);
    }
    phydisconnect(left);
    phywake(left); /* Wake remains a valid callback until final free, even after local disconnect. */
    phyfree(left);
    phyfree(right);
    return good && stats.destroyed == 2;
}


/* EN/RU: No arguments; report the first failing driver-contract group and return 1, else 0.
 * Аргументов нет; сообщить первую ошибочную группу и вернуть 1, иначе 0.
 */
int main(void)
{
    if (!driver_contract())
    {
        fprintf(stderr, "memory PHY prefix/backpressure/drain contract failed\n");
        return 1;
    }
    if (!uncertain_prefix())
    {
        fprintf(stderr, "memory PHY uncertainty contract failed\n");
        return 1;
    }
    if (!wait_contract())
    {
        fprintf(stderr, "memory PHY deadline/concurrent wake contract failed\n");
        return 1;
    }
    puts("PASS memory PHY contracts: bounded prefixes, backpressure, drain/EOF, uncertainty and sticky/concurrent wake");
    return 0;
}
