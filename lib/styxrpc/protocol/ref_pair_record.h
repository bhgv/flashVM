/* Copyright (c) 2026 bhgv
 * EN: Copy values and call-local identity for two managed references across Styx.
 * RU: Передача значений и идентичности двух управляемых ссылок через Styx.
 */
#ifndef STYX_REF_PAIR_RECORD_H
#define STYX_REF_PAIR_RECORD_H

#include "tuple_record.h"

typedef struct StyxRefPair
{
    size_t count; /* One object for argument indices 0,0 or two for 0,1. */
    StyxTuple objects[2]; /* Object states in first-appearance order. */
    StyxTuple result; /* Ordinary return, present in replies only. */
} StyxRefPair;


/* EN: Encode value as a request or success reply selected by tag. Bound bytes
 * by capacity and report used on success. Return zero without writing on refusal.
 * RU: Закодировать value как запрос или ответ по tag. Ограничить bytes capacity,
 * выдать used при успехе. При отказе не записывать результат. */
int styxrefpairwrite(unsigned long tag, const StyxRefPair *value, void *bytes, size_t capacity, size_t *used);


/* EN: Validate size bytes, tag and all object IDs before modifying value.
 * RU: Проверить size байтов, tag и все номера объектов до изменения value. */
int styxrefpairread(unsigned long tag, const void *bytes, size_t size, StyxRefPair *value);

#endif
