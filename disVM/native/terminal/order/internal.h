/* Copyright (c) 2026 bhgv
 * EN: Mutex-protected tag-local queues, immutable payload copies and explicit per-direction rollover state.
 * RU: Очереди тегов под mutex, неизменяемые копии данных и отдельное состояние сброса каждого направления.
 */
#ifndef STYX_ORDER_INTERNAL_H
#define STYX_ORDER_INTERNAL_H
#define _POSIX_C_SOURCE 200809L
#include "order.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/* EN/RU: One owned checked packet payload, staged in a single tag's held or ready FIFO/list.
 * Одна owned checked packet payload в held либо ready list одного тега.
 */
typedef struct OrderRecord
{
    struct OrderRecord *next; /* Owned list continuation. */
    unsigned long long ordinal; /* Exact unsigned64 sequence without generation field. */
    size_t size; /* Exact copied payload size. */
    unsigned char bytes[]; /* Native-owned opaque body after verified S-record tag. */
} OrderRecord;


/* EN/RU: One tag in this engine namespace; RX/TX and operation histories are independent.
 * Один tag engine namespace, RX/TX и operation histories независимы.
 */
typedef struct OrderTag
{
    struct OrderTag *next; /* Owned registry continuation. */
    unsigned long tag; /* Uint32 original S-record ID. */
    unsigned used[2]; /* Direction was seeded or accepted actual traffic. */
    unsigned exhausted[2]; /* Counter reached MAX; no implicit reuse of0. */
    unsigned pending[2]; /* Native reset proposal exists in this direction. */
    unsigned long long operation[2]; /* Exact pending local operation ID. */
    unsigned long long last[2]; /* Greatest admitted reset proposal ID. */
    unsigned long long done[2]; /* Most recent resolved proposal. */
    int accepted[2]; /* Cached most recent confirmation result. */
    unsigned long long boundary[2]; /* Final old ordinal for this proposal. */
    unsigned long long expected; /* Next contiguous RX sequence; MAX retained on exhaustion. */
    unsigned long long transmit; /* Next TX spool number; MAX retained on exhaustion. */
    unsigned long long tx_last; /* Last successfully spooled ordinal. */
    int tx_spooled; /* TX boundary is meaningful only after actual spool success. */
    unsigned long long consumed; /* Last successfully copied RX ordinal. */
    int rx_consumed; /* Consumed watermark is meaningful. */
    OrderRecord *held; /* Owned sorted future records for this tag only. */
    OrderRecord *head; /* Owned ready FIFO. */
    OrderRecord *tail; /* Borrowed newest ready node. */
    size_t ready_count; /* Current unread contiguous records. */
    size_t held_count; /* Current future records, not an application queue capacity. */
} OrderTag;


/* EN/RU: Shared native engine, no protocol worker or guest callback inside this object.
 * Общий native engine без protocol worker либо guest callback внутри объекта.
 */
struct StyxOrder
{
    pthread_mutex_t lock; /* Protect registry/counters/queues and closed status. */
    pthread_cond_t changed; /* Per-consumer predicates, native monotonic readiness. */
    OrderTag *tags; /* Owned dynamic tag registry. */
    unsigned limit; /* Original S-record body bound. */
    unsigned long long window; /* Maximum future sequence displacement, strictly below half-range. */
    int closed; /* Connection owner retired; ready messages remain drainable. */
};


/* EN/RU: Lookup/optionally allocate one tag under engine mutex; NULL missing/allocation failure.
 * Найти/allocate tag под mutex, NULL missing/allocation failure.
 */
OrderTag *ordertag(StyxOrder *order, unsigned long tag, int create);


/* EN/RU: Copy selected rollover readiness under engine mutex, exact pending ID, no mutation.
 * Readiness rollover под mutex, exact pending ID, без mutation.
 */
int orderready(OrderTag *tag, unsigned direction, unsigned long long operation);
#endif
