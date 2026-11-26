/* Copyright (c) 2026 bhgv
 * EN: Hold scalar frames behind live, session-local cell capabilities.
 * RU: Хранить скалярные кадры за живыми полномочиями ячеек сессии.
 */
#include "named_cell_store.h"
#include <stdlib.h>
#include <string.h>

struct NamedCell
{
    NamedCell *next; /* Link in the registry of live cells. */
    size_t refs; /* Leases plus queued transfers. */
    size_t size; /* Count of bytes in the owned frame. */
    unsigned char *frame; /* Current complete scalar value. */
    unsigned char token[STYXRPC_CELL_TOKEN]; /* Exact identity until final release. */
};


/* EN/RU: See named_cell_store.h. / См. named_cell_store.h. */
NamedCell *styxrpc_cells_create(NamedCells *cells, const unsigned char *nonce,
    const void *frame, size_t size, unsigned char *token)
{
    if (cells->latest == UINT64_MAX)
    {
        return NULL;
    }
    NamedCell *cell = malloc(sizeof(*cell));
    unsigned char *copy = malloc(size);
    if (!cell || !copy)
    {
        free(cell);
        free(copy);
        return NULL;
    }
    memcpy(copy, frame, size);
    memcpy(token, nonce, STYXRPC_CELL_NONCE);
    uint64_t serial = ++cells->latest;
    for (size_t i = STYXRPC_CELL_TOKEN; i > STYXRPC_CELL_NONCE; --i)
    {
        token[i - 1] = (unsigned char)serial;
        serial >>= 8;
    }
    memcpy(cell->token, token, STYXRPC_CELL_TOKEN);
    cell->frame = copy;
    cell->size = size;
    cell->refs = 1;
    cell->next = cells->head;
    cells->head = cell;
    return cell;
}


/* EN/RU: See named_cell_store.h. / См. named_cell_store.h. */
NamedCell *styxrpc_cells_attach(NamedCells *cells, const unsigned char *token)
{
    for (NamedCell *cell = cells->head; cell; cell = cell->next)
    {
        if (cell->refs < SIZE_MAX && !memcmp(token, cell->token, STYXRPC_CELL_TOKEN))
        {
            ++cell->refs;
            return cell;
        }
    }
    return NULL;
}


/* EN/RU: See named_cell_store.h. / См. named_cell_store.h. */
void styxrpc_cells_release(NamedCells *cells, NamedCell *cell)
{
    if (cell && !--cell->refs)
    {
        NamedCell **link = &cells->head;
        while (*link != cell)
        {
            link = &(*link)->next;
        }
        *link = cell->next;
        free(cell->frame);
        free(cell);
    }
}


/* EN/RU: See named_cell_store.h. / См. named_cell_store.h. */
unsigned char *styxrpc_cells_get(const NamedCell *cell, size_t *size)
{
    unsigned char *copy = malloc(cell->size);
    if (copy)
    {
        memcpy(copy, cell->frame, cell->size);
        *size = cell->size;
    }
    return copy;
}


/* EN/RU: See named_cell_store.h. / См. named_cell_store.h. */
void styxrpc_cells_set(NamedCell *cell, unsigned char *frame, size_t size)
{
    free(cell->frame);
    cell->frame = frame;
    cell->size = size;
}
