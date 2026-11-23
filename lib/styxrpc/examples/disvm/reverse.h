/* Copyright (c) 2026 bhgv
 * EN: Shared declarations for the C-hosted reverse process tree example.
 * RU: Объявления примера встречного дерева процессов на C.
 */
#ifndef STYXRPC_EXAMPLE_REVERSE_H
#define STYXRPC_EXAMPLE_REVERSE_H
#include "styxrpc/collection.h"
#include "hybrid.h"
#include <pthread.h>

typedef struct ReverseState
{
    int32_t value;      /* Client-owned variable changed by the Dis launch. */
    unsigned char byte; /* Byte written by the guest through its fd 7. */
    unsigned char output; /* Byte written by the guest through its standard output. */
    int parked;         /* Atomically published once the guest enters its loop. */
    StyxRpcPrograms tree;          /* Reverse registration context. */
    StyxIntArray vector;         /* Registered positional collection. */
    StyxIntArray keyed;          /* Registered numeric-key collection. */
    int32_t values[4];           /* C-owned vector storage. */
    uint32_t keys[2];            /* C-owned keyed names. */
    int32_t mapped[2];           /* Values corresponding to keys. */
    Hybrid hybrid;                      /* C-owned dense and keyed table. */
    StyxRpcRegistration nodes[44]; /* Fixed registration storage. */
    StyxRpcServer *server; /* Owned by the dispatcher once its thread is started. */
} ReverseState;

/* EN: Fill state with known caller-owned resources before starting dispatch.
 * RU: Зарегистрировать ресурсы клиента в state до запуска диспетчера. */
int register_reverse(ReverseState *state);

/* EN: Move dispatcher ownership to a peer thread until the connection stops.
 * RU: Передать диспетчер потоку до завершения соединения. */
void *run_reverse(void *context);
#endif
