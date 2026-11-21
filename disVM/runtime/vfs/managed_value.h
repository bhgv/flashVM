/* Copyright (c) 2026 bhgv
 * EN: Construct receiver-local Dis values from validated portable Styx records.
 * RU: Создание локальных значений Dis из проверенных переносимых записей Styx.
 */
#ifndef DVM_STYX_MANAGED_VALUE_H
#define DVM_STYX_MANAGED_VALUE_H
#include "flashvm/srecord.h"


/* [GC:] EN: Decode one leaf of type s or a[y]. Borrow type and record, allocate
 * on the current ACTIVE resident, and return one owned reference or H for nil.
 * Return zero and preserve result on malformed data. Allocator errors propagate.
 * The recipient must root the returned object before leaving GC admission.
 * RU: Декодировать s или a[y]. Тип и запись заимствуются; выделить объект в
 * текущем ACTIVE resident. Вернуть владеющую ссылку либо H для nil. При
 * неверных данных вернуть 0, сохранив result. Ошибки выделения передаются
 * вызывающему. Вызывающий укореняет объект до выхода из допуска GC. */
int dvm_styx_managed_new(const char *type, const FVM_SRecord *record, void **result);


/* [GC:] EN: Encode a rooted s or a[y] value into disjoint native storage.
 * Observe a stable String owner or borrowed array with immutable header.
 * Payload is snapped before encoding. On failure bytes and used are preserved.
 * RU: Кодировать укоренённые s или a[y] в отдельный нативный буфер.
 * Строка удерживается стабильным владельцем, заголовок массива неизменяем.
 * Данные копируются до кодирования. При отказе bytes и used сохраняются. */
int dvm_styx_managed_write(const char *type, void *value, void *bytes, size_t capacity, size_t *used);
#endif
