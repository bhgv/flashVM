/* Copyright (c) 2026 bhgv
 * EN: Exercise actual process-root membership across unmapped peer retirement.
 * RU: Проверять настоящий список корней при снятии и unmap памяти соседнего Prog.
 */
#include "lib9.h"
#include "dat.h"
#include "interp.h"
#include "runtime_gc.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>

static Prog anchor; /* Stable independently retained queried identity. */
static pthread_mutex_t sync = PTHREAD_MUTEX_INITIALIZER; /* Test milestone mutex. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Read/unmap/resume ordering. */
static int read_done; /* Query completed, or old traversal borrowed a pointer. */
static int removed; /* Peer was really removed and unmapped. */
static unsigned live_writers; /* Atomic native writer completion count. */
static unsigned long queries; /* Atomic successful independent membership observations. */
static size_t extent; /* Complete page-aligned native Prog mapping extent. */


/* EN: Require condition or terminate with borrowed message; no hidden side effects.
 * RU: Проверить condition либо завершить с message; без скрытых побочных действий. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL process roots: %s\n", message);
        exit(12);
    }
}


/* EN: Allocate a real independently owned native Prog mapping, or fail.
 * RU: Выделить настоящее независимо принадлежащее mapping Prog либо завершить. */
static Prog *create_peer(void)
{
    void *memory = mmap(NULL, extent, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    require(memory != MAP_FAILED, "native Prog mmap succeeds");
    return memory;
}


/* EN: Model observer's old borrowed-head walk or the new completed scalar query.
 * argument is unused; anchor lifetime is held by main until the reader joins.
 * RU: Моделировать старый обход головы либо новый законченный scalar-запрос.
 * argument не используется; main удерживает anchor до join читателя. */
static void *delayed_reader(void *argument)
{
    (void)argument;
#if CONTROL
    Prog *cursor = dvm_runtime_process_roots();
#else
    int found = dvm_runtime_process_root_contains(&anchor);
#endif
    pthread_mutex_lock(&sync);
    read_done = 1;
    pthread_cond_broadcast(&changed);
    while (!removed)
    {
        pthread_cond_wait(&changed, &sync);
    }
    pthread_mutex_unlock(&sync);
#if CONTROL
    fprintf(stderr, "ROOT_WALK_CONTROL peer really unmapped before borrowed traversal\n");
    int found = 0;
    for (; cursor; cursor = cursor->next)
    {
        found |= cursor == &anchor;
    }
#endif
    require(found && dvm_runtime_process_root_contains(&anchor), "queried anchor remains independently alive");
    return NULL;
}


/* EN: Repeatedly publish/remove native peers, then unmap storage immediately.
 * argument is unused; root mutex must exclude every concurrent query traversal.
 * RU: Публиковать/снимать native-соседей, сразу делая unmap памяти.
 * argument не используется; root mutex исключает все параллельные обходы. */
static void *writer(void *argument)
{
    (void)argument;
    for (unsigned at = 0; at < 512; at++)
    {
        Prog *peer = create_peer();
        dvm_runtime_process_root_add(peer);
        require(dvm_runtime_process_root_remove(peer), "published peer is removed exactly once");
        require(!munmap(peer, extent), "retired peer mapping is released");
    }
    __atomic_fetch_sub(&live_writers, 1, __ATOMIC_RELEASE);
    return NULL;
}


/* EN: Query while writers retire peers; no borrowed peer address can escape.
 * argument is unused; every result concerns the main-owned anchor only.
 * RU: Проверять при снятии соседей; адрес соседа не должен выйти наружу.
 * argument не используется; результат относится лишь к anchor владельца main. */
static void *reader(void *argument)
{
    (void)argument;
    unsigned at = 0;
    while (at < 2048 || __atomic_load_n(&live_writers, __ATOMIC_ACQUIRE))
    {
        require(dvm_runtime_process_root_contains(&anchor), "concurrent retained anchor is always found");
        __atomic_fetch_add(&queries, 1, __ATOMIC_RELAXED);
        at++;
    }
    return NULL;
}


/* EN: Exercise delayed retirement, four writers/readers and absent/cycle cases.
 * No CLI arguments. Own every thread/mapping until join/unmap; return0 on success.
 * RU: Проверить задержку, четыре писателя/читателя и случаи отсутствия/цикла.
 * Без CLI-аргументов. Владеть потоками/mapping до join/unmap; вернуть0 при успехе. */
int main(void)
{
    struct rlimit core = {0, 0};
    setrlimit(RLIMIT_CORE, &core);
    long page = sysconf(_SC_PAGESIZE);
    require(page > 0, "native page size is available");
    extent = ((sizeof(Prog) + (size_t)page - 1) / (size_t)page) * (size_t)page;
    dvm_runtime_process_root_add(&anchor);
    Prog *peer = create_peer();
    dvm_runtime_process_root_add(peer);
    pthread_t delayed;
    require(!pthread_create(&delayed, NULL, delayed_reader, NULL), "delayed reader starts");
    pthread_mutex_lock(&sync);
    while (!read_done)
    {
        pthread_cond_wait(&changed, &sync);
    }
    require(dvm_runtime_process_root_remove(peer), "delayed peer leaves real registry");
    require(!munmap(peer, extent), "delayed peer storage is really inaccessible");
    removed = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&sync);
    require(!pthread_join(delayed, NULL), "delayed reader joins");
    live_writers = 4;
    pthread_t writers[4];
    pthread_t readers[4];
    for (unsigned at = 0; at < 4; at++)
    {
        require(!pthread_create(&writers[at], NULL, writer, NULL), "writer starts");
        require(!pthread_create(&readers[at], NULL, reader, NULL), "reader starts");
    }
    for (unsigned at = 0; at < 4; at++)
    {
        require(!pthread_join(writers[at], NULL), "writer joins");
        require(!pthread_join(readers[at], NULL), "reader joins");
    }
    anchor.next = &anchor;
    require(!dvm_runtime_process_root_contains(&anchor), "cycle is rejected without an infinite walk");
    anchor.next = NULL;
    require(dvm_runtime_process_root_remove(&anchor), "last anchor withdraws");
    require(!dvm_runtime_process_root_contains(&anchor) && !dvm_runtime_process_root_contains(NULL), "absent identities are rejected");
    fprintf(stderr, "PASS real process roots peers=2049 queries=%lu unmapped=2049 cycle=checked\n", queries);
    return 0;
}
