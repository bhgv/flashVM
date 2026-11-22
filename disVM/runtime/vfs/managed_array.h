/* Copyright (c) 2026 bhgv
 * EN: Bridge primitive Dis arrays to typed Styx child records.
 * RU: Передача примитивных массивов Dis типизированными записями Styx.
 */
#ifndef DVM_STYX_MANAGED_ARRAY_H
#define DVM_STYX_MANAGED_ARRAY_H
#include "managed_value.h"


/* [GC:] EN: Construct a receiver-local primitive array from one validated
 * sequence record. On failure preserve result; successful non-nil results
 * transfer one owned array to the caller.
 * RU: Создать локальный массив из записи последовательности. При отказе
 * сохранить result; при успехе передать одну владеющую ссылку.
 */
int dvm_styx_primitive_array_new(char element, const FVM_SRecord *record, void **result);


/* EN: Return whether the element code has a canonical scalar array descriptor.
 * RU: Проверить, есть ли канонический дескриптор для кода элемента.
 */
int dvm_styx_primitive_array_supported(char element);


/* [GC:] EN: Encode a rooted primitive array into a complete sequence record.
 * Snapshot its data before encoding; preserve used on failure.
 * RU: Кодировать укоренённый примитивный массив одной записью. Сначала
 * скопировать данные; при отказе сохранить used.
 */
int dvm_styx_primitive_array_write(char element, void *value, void *bytes,
    size_t capacity, size_t *used);
/* EN: Construct or encode an array with strings, arrays or tuple elements.
 * RU: Создать либо закодировать массив строк, массивов или кортежей. */
int dvm_styx_composite_array_new(const char *type, const FVM_SRecord *record, void **result);

int dvm_styx_composite_array_write(const char *type, void *value, void *bytes,
    size_t capacity, size_t *used);


/* EN: Read or write exactly one element in the precomputed singleton schema.
 * RU: Прочитать или записать ровно один элемент по заранее вычисленной схеме. */
int dvm_styx_array_element_read(const char *shape, const unsigned char *record, size_t size,
    const StyxScalarLayout *layout, DVM_StyxValueFrame *element);

int dvm_styx_array_element_write(const char *shape, const DVM_StyxValueFrame *element,
    void *bytes, size_t capacity, size_t *used);
#endif
