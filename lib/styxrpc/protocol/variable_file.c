/* Copyright (c) 2026 bhgv
 * EN: Full-value I/O for callback-backed scalar files in a Styx variable tree.
 * RU: Ввод-вывод целых значений файлов скаляров дерева переменных Styx.
 */
#include "variable_file.h"


/* EN/RU: Return a snapshot in network byte order without splitting the value.
 * Вернуть снимок в сетевом порядке байтов без разбиения значения. */
int styxintvariableread(const StyxIntVariable *variable, uint64_t offset, void *bytes, size_t *size)
{
    if (!variable || !variable->load || !size || (*size && !bytes))
    {
        return 0;
    }
    if (offset >= STYX_VARIABLE_INT_SIZE || !*size)
    {
        *size = 0;
        return 1;
    }
    int32_t value;
    if (offset || *size < STYX_VARIABLE_INT_SIZE || !variable->load(variable->state, &value))
    {
        return 0;
    }
    unsigned char *out = bytes;
    uint32_t bits = (uint32_t)value;
    for (size_t i = 0; i < STYX_VARIABLE_INT_SIZE; i++)
    {
        out[i] = (unsigned char)(bits >> (24 - 8 * i));
    }
    *size = STYX_VARIABLE_INT_SIZE;
    return 1;
}


/* EN/RU: Decode the exact signed value before forwarding one update.
 * Декодировать точное знаковое значение перед передачей одного изменения. */
int styxintvariablewrite(const StyxIntVariable *variable, uint64_t offset, const void *bytes, size_t size)
{
    if (!variable || !variable->store || offset || !bytes || size != STYX_VARIABLE_INT_SIZE)
    {
        return 0;
    }
    const unsigned char *in = bytes;
    uint32_t bits = 0;
    for (size_t i = 0; i < STYX_VARIABLE_INT_SIZE; i++)
    {
        bits = (bits << 8) | in[i];
    }
    int32_t value = bits <= INT32_MAX ? (int32_t)bits : -1 - (int32_t)(UINT32_MAX - bits);
    return variable->store(variable->state, value) == 1;
}
