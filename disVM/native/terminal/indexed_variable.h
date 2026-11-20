/* Copyright (c) 2026 bhgv
 * EN: One live index selection per exported process and variable.
 * RU: Один активный выбор индекса на переменную выданного процесса.
 */
#ifndef STYX_INDEXED_VARIABLE_H
#define STYX_INDEXED_VARIABLE_H
#include "variable_file.h"

typedef struct StyxIntArray
{
    void *state; /* Borrowed owner context. */
    uint32_t length; /* Stable zero-based element count. */
    int (*load)(void *, uint32_t, int32_t *); /* Read an element. */
    int (*store)(void *, uint32_t, int32_t); /* Update an element. */
    const void *client; /* Connection owning the active selection, if any. */
    uint64_t generation; /* Monotonic identity of the .idx opening. */
    uint32_t index; /* Selected element. */
    int selected; /* A complete valid index has been written. */
} StyxIntArray;

uint64_t styxintarrayopen(StyxIntArray *array, const void *client, int selector);
void styxintarrayclose(StyxIntArray *array, uint64_t generation);
int styxintarrayselect(StyxIntArray *array, uint64_t generation, uint64_t offset, const void *bytes, size_t size);
int styxintarrayread(StyxIntArray *array, uint64_t generation, uint64_t offset, void *bytes, size_t *size);
int styxintarraywrite(StyxIntArray *array, uint64_t generation, uint64_t offset, const void *bytes, size_t size);
#endif
