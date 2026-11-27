/* Copyright (c) 2026 bhgv
 * EN: Internal mapped construction and bounded encoding of Limbo value lists.
 * RU: Внутренняя сборка размеченных списков Limbo и ограниченное кодирование.
 */
#ifndef DVM_STYX_MANAGED_LIST_H
#define DVM_STYX_MANAGED_LIST_H
#include "managed_value.h"


/* [GC:] EN: Decode a checked nonempty list record to an owned native chain.
 * RU: Декодировать проверенную непустую запись в собственный нативный список. */
int dvm_styx_list_new(const char *type, const FVM_SRecord *record, void **out);


/* [GC:] EN: Encode a rooted non-nil native chain as a sequence of typed children.
 * RU: Закодировать удерживаемый непустой список последовательностью типизированных записей. */
int dvm_styx_list_write(const char *type, void *value, void *bytes, size_t capacity, size_t *used);
#endif
