/* Copyright (c) 2026 bhgv
 * EN: Serialized process-scoped .idx and .val selection state.
 * RU: Последовательный выбор .idx и .val в контексте процесса.
 */
#include "indexed_variable.h"


/* EN/RU: Borrow the live array and bind an open to its connection and generation.
 * Заимствовать массив и связать открытие с соединением и поколением выбора. */
uint64_t styxintarrayopen(StyxIntArray *array, const void *client, int selector)
{
    if (!array || !client)
    {
        return 0;
    }
    if (!selector)
    {
        return array->client == client && array->selected ? array->generation : 0;
    }
    if (array->client || array->generation == UINT64_MAX)
    {
        return 0;
    }
    array->client = client;
    return ++array->generation;
}


/* EN/RU: Release the matching selector and invalidate all of its value opens.
 * Освободить совпадающий выбор и отозвать все связанные открытия значения. */
void styxintarrayclose(StyxIntArray *array, uint64_t generation)
{
    if (array && generation && array->generation == generation)
    {
        array->client = NULL;
        array->selected = 0;
    }
}


/* EN/RU: Verify a live generation and, for value I/O, its selected index.
 * Проверить живое поколение и выбранный индекс для операций значения. */
static int active(const StyxIntArray *array, uint64_t generation)
{
    return array && array->client && generation && array->generation == generation;
}


/* EN/RU: Accept a single full big-endian index in the owner's extent.
 * Принять один полный индекс со старшим байтом первым в пределах массива. */
int styxintarrayselect(StyxIntArray *array, uint64_t generation, uint64_t offset, const void *bytes, size_t size)
{
    if (!active(array, generation) || array->selected || offset || !bytes || size != STYX_VARIABLE_INT_SIZE)
    {
        return 0;
    }
    const unsigned char *in = bytes;
    uint32_t index = ((uint32_t)in[0] << 24) | ((uint32_t)in[1] << 16) | ((uint32_t)in[2] << 8) | in[3];
    if (index >= array->length)
    {
        return 0;
    }
    array->index = index;
    array->selected = 1;
    return 1;
}


/* EN/RU: Read the selected element from the borrowed native owner.
 * Прочитать выбранный элемент у заимствованного нативного владельца. */
static int get(void *state, int32_t *value)
{
    StyxIntArray *array = state;
    return array->load && array->load(array->state, array->index, value) == 1;
}


/* EN/RU: Store into the selected owner element exactly once.
 * Один раз записать выбранный элемент владельца. */
static int put(void *state, int32_t value)
{
    StyxIntArray *array = state;
    return array->store && array->store(array->state, array->index, value) == 1;
}


/* EN/RU: Read one signed value through a still-live .val generation.
 * Прочитать знаковое значение через ещё действующее открытие .val. */
int styxintarrayread(StyxIntArray *array, uint64_t generation, uint64_t offset, void *bytes, size_t *size)
{
    const StyxIntVariable variable = {array, get, NULL};
    return active(array, generation) && array->selected && styxintvariableread(&variable, offset, bytes, size);
}


/* EN/RU: Update one signed value through a still-live .val generation.
 * Изменить знаковое значение через ещё действующее открытие .val. */
int styxintarraywrite(StyxIntArray *array, uint64_t generation, uint64_t offset, const void *bytes, size_t size)
{
    const StyxIntVariable variable = {array, NULL, put};
    return active(array, generation) && array->selected && styxintvariablewrite(&variable, offset, bytes, size);
}
