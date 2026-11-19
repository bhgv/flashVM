/* Copyright (c) 2026 bhgv
 * EN: Private shared-ring lifetime and binary native wake latch for the memory driver.
 * RU: Частное время жизни shared ring и бинарный native wake latch драйвера памяти.
 * standalone-reviewed: all shared byte state is protected; no wire/native layout overlay.
 */
#ifndef EXAMPLE_MEMORY_INTERNAL_H
#define EXAMPLE_MEMORY_INTERNAL_H
#include "memory.h"
#include <pthread.h>
#include <stdint.h>


typedef struct MemoryLink MemoryLink; /* Shared allocation, pinned by both endpoint owners. */


/* EN/RU: One immutable side binding and atomic local wake state.
 * Неизменяемая привязка стороны и atomic local wake state.
 */
typedef struct MemoryEnd
{
    MemoryLink *link;   /* Borrowed from the shared allocation pinned by this endpoint. */
    unsigned side;     /* 0 or 1, never a protocol/client tag. */
    uint32_t event;    /* Binary futex latch, not a wrapping sequence counter. */
    unsigned wake;     /* Sticky explicit wake, consumed only by this endpoint's wait owner. */
    size_t written;    /* Owner-only confirmed prefix count for the optional injected cutoff. */
    size_t cut_after;  /* Zero normally; nonzero only for the explicit failure demonstration. */
} MemoryEnd;


/* EN/RU: Both endpoints must retire I/O/wake users before their owned destruction.
 * Оба endpoints завершают I/O/wake users перед уничтожением владельцем.
 */
struct MemoryLink
{
    pthread_mutex_t lock; /* Only ring/connection/reference state; no native wait or I/O under this lock. */
    unsigned references;  /* Starts at two; last destroy releases this entire allocation. */
    int open[2];           /* Persistent peer disconnect indication under lock. */
    size_t head[2];        /* Oldest incoming byte offset for each side. */
    size_t used[2];        /* Count of valid incoming bytes, 0..MEMORY_CAPACITY. */
    unsigned char bytes[2][MEMORY_CAPACITY]; /* Incoming rings; no frame boundaries or whole-tree buffering. */
    MemoryEnd ends[2];     /* Callback state remains valid while either endpoint pins the allocation. */
    MemoryStats *stats;    /* Borrowed observations outlive both final destroys. */
};


/* EN: Increment borrowed counter with relaxed atomics; observations are consumed after users retire.
 * RU: Увеличить counter relaxed atomic; наблюдения читаются после завершения users.
 */
void memory_count(unsigned long *counter, unsigned long amount);


/* EN: Set the endpoint's binary event latch and wake its native owner; no waiting/descriptor access.
 * RU: Установить binary event latch end и разбудить owner; без ожидания/доступа к fd.
 */
void memory_notify(MemoryEnd *end);


/* EN/RU: Callback implementations; public contract and ownership are defined in phy.h.
 * Реализации callbacks; публичный контракт и владение определены в phy.h.
 */
PhyTransfer memory_read(void *state, void *bytes, size_t size);
PhyTransfer memory_write(void *state, const void *bytes, size_t size);
int memory_wait(void *state, unsigned wanted, int timeout, unsigned *ready);
void memory_wake(void *state);
void memory_disconnect(void *state);
void memory_destroy(void *state);
#endif
