/* Copyright (c) 2026 bhgv
 * EN: Shared declarations for the C-hosted reverse process tree example.
 * RU: Объявления примера встречного дерева процессов на C.
 */
#ifndef STYXRPC_EXAMPLE_REVERSE_H
#define STYXRPC_EXAMPLE_REVERSE_H
#include "styxrpc/serve.h"
#include <pthread.h>

typedef struct ReverseState
{
    int32_t value;      /* Client-owned variable changed by the Dis launch. */
    unsigned char byte; /* Byte written by the guest through its fd 7. */
    unsigned char output; /* Byte written by the guest through its standard output. */
    int parked;         /* Atomically published once the guest enters its loop. */
    StyxRpcServer *server; /* Owned by the dispatcher once its thread is started. */
} ReverseState;

extern const StyxRpcTree reverse_tree; /* Exact set of client-side grants. */

/* EN: Move dispatcher ownership to a peer thread until the connection stops.
 * RU: Передать диспетчер потоку до завершения соединения. */
void *run_reverse(void *context);
#endif
