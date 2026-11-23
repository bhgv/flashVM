/* Copyright (c) 2026 bhgv
 * EN: Small application-owned hybrid table for the named launch example.
 * RU: Небольшая гибридная таблица приложения для примера именованного запуска.
 */
#ifndef STYXRPC_EXAMPLE_HYBRID_H
#define STYXRPC_EXAMPLE_HYBRID_H
#include "styxrpc/bigtable.h"

typedef struct HybridEntry
{
    StyxRpcBigKey key; /* Stable key borrowing text below. */
    char text[8];      /* Fixture's bounded text keys, copied at insertion. */
    int64_t value;    /* Signed big scalar. */
} HybridEntry;

typedef struct Hybrid
{
    StyxRpcBigTable table;  /* Published callback descriptor. */
    int64_t dense[6];       /* Native positional values. */
    size_t length;          /* Live dense length. */
    HybridEntry sparse[4];  /* Text and sparse numeric namespace. */
    size_t count;           /* Live count of sparse entries. */
} Hybrid;

/* EN: Initialize a native hybrid table and publish its callbacks at .var/hybrid.
 * RU: Инициализировать гибридную таблицу и опубликовать её в .var/hybrid. */
int register_hybrid(StyxRpcPrograms *programs, uint32_t pid, Hybrid *state);
#endif
