/* Copyright (c) 2026 bhgv
 * EN: Example graph-facing export and immutable per-fid reply state.
 * RU: Графовый экспорт примера и неизменяемый ответ отдельного fid.
 */
#ifndef STYXRPC_EXAMPLE_GRAPH_DEVICE_H
#define STYXRPC_EXAMPLE_GRAPH_DEVICE_H
#include "styxrpc/graph.h"

extern const StyxRpcSchema bump_schema; /* ref Box, with nested ref Cell. */
#define BUMP_SIGNATURE UINT32_C(0xdd53d837) /* Limbo-compiled bump signature. */

typedef struct BumpCall
{
    unsigned char request[256]; /* Full original call retained through reply. */
    unsigned char reply[256];   /* Immutable reply. */
    size_t staged;              /* Acknowledged prefix in bytes. */
    size_t ready;               /* Complete encoded reply size, or zero. */
    int refused;                /* Sticky malformed-request refusal. */
} BumpCall;

/* EN: Stage bytes once and increment the nested object after the complete call.
 * RU: Принять байты и увеличить вложенный объект после полного вызова. */
int bump_write(BumpCall *call, size_t offset, const void *bytes, size_t size);

/* EN: Copy a positional prefix of the immutable reply.
 * RU: Скопировать позиционный фрагмент неизменяемого ответа. */
int bump_read(const BumpCall *call, size_t offset, void *bytes, size_t capacity, size_t *size);

#endif
