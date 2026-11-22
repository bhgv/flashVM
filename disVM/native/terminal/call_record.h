/* Copyright (c) 2026 bhgv
 * EN: Ordered typed S-records for a mounted Dis argument list or return.
 * RU: Упорядоченные типизированные S-записи аргументов или результата Dis.
 */
#ifndef STYX_CALL_RECORD_H
#define STYX_CALL_RECORD_H

#include <stddef.h>
#include <stdint.h>
#include "flashvm/srecord.h"
#include "load_profile.h"

#define STYX_CALL_VALUES 8U /* Maximum flattened scalar cells in a frame. */
#define STYX_CALL_DEPTH 4U /* Maximum tuple nesting, including the outer list. */

typedef struct StyxCallValues
{
    size_t count; /* Number of scalar cells in traversal order. */
    int64_t cells[STYX_CALL_VALUES]; /* Signed Limbo values, widened to a common cell. */
} StyxCallValues;

extern const char styxcall_list_args[]; /* Three argument positions, with a nested tuple. */
extern const char styxcall_list_result[]; /* One nested return tuple. */


/* EN: Write all values according to trusted shape i/b/p/(...), with one outer tag.
 * RU: Записать все values по доверенной схеме i/b/p/(...) с внешним тегом tag.
 * bytes/capacity bound the caller's buffer; used receives the size on success.
 * bytes/capacity задают буфер; used получает размер при успехе. */
int styxcallrecordwrite(unsigned long tag, const char *shape, const StyxCallValues *values,
    void *bytes, size_t capacity, size_t *used);


/* EN: Verify exactly size bytes against tag and trusted shape, then publish values.
 * RU: Проверить ровно size байтов по тегу и доверенной схеме, затем выдать values. */
int styxcallrecordread(unsigned long tag, const char *shape, const void *bytes, size_t size,
    StyxCallValues *values);

typedef struct StyxScalarLayout
{
    size_t size; /* Exact byte extent of the NC64 layout. */
    size_t count; /* Number of scalar leaves. */
    size_t offset[STYX_CALL_VALUES]; /* Byte offsets in schema order. */
    unsigned char width[STYX_CALL_VALUES]; /* Native leaf widths. */
    unsigned pointers; /* Bit i marks a managed reference at leaf i. */
} StyxScalarLayout;


/* EN: Validate a value-only schema and compute its NC64 layout.
 * RU: Проверить схему значений и вычислить раскладку NC64. */
int styxscalar_layout(const char *shape, StyxScalarLayout *layout);


/* EN: Compute NC64 field offsets and identify managed roots without reading a frame.
 * RU: Вычислить смещения NC64 и отметить управляемые корни без чтения кадра. */
int styxmanaged_layout(const char *shape, StyxScalarLayout *layout);


/* EN: Copy only a pointer-free NC64 buffer and widen packed byte leaves.
 * RU: Скопировать поля буфера NC64 без указателей и расширить поля byte. */
void styxscalar_unpack(const StyxScalarLayout *layout, const void *frame, StyxCallValues *values);


/* EN: Stage pointer-free fields in an NC64 buffer and zero its padding.
 * RU: Записать поля без указателей в буфер NC64 и обнулить заполнители. */
void styxscalar_pack(const StyxScalarLayout *layout, const StyxCallValues *values, void *frame);

#define STYX_CALL_INT_TAG 1UL /* Signed Limbo int leaf. */
#define STYX_CALL_BIG_TAG 2UL /* Signed Limbo big leaf. */
#define STYX_CALL_TUPLE_TAG 4UL /* Ordered nested tuple. */
#define STYX_CALL_REAL_TAG 5UL /* IEEE 754 binary64 bits. */
#define STYX_CALL_BYTE_TAG 6UL /* Unsigned Limbo byte leaf. */
#define STYX_MANAGED_LIST_LIMIT 65536U /* Maximum complete managed value record. */
#define STYX_MANAGED_STRING_TAG 7UL /* Counted UTF-8 bytes. */
#define STYX_MANAGED_ARRAY_TAG 8UL /* Sequence of nested records of one declared element type. */
#define STYX_MANAGED_NIL_TAG 9UL /* Empty payload for a nil managed leaf. */
#define STYX_MANAGED_REF_TAG 10UL /* Index byte, then fields on first occurrence. */


/* EN: Extract one ref ADT's non-reference field schema.
 * RU: Получить схему полей одного ref ADT без вложенных ref. */
int styxmanaged_ref_fields(const char *shape, char fields[STYX_SHAPE_MAX + 1]);

/* EN: Borrow checked ref nodes in flattened argument order.
 * RU: Заимствовать проверенные узлы ref в развёрнутом порядке аргументов. */
int styxmanaged_ref_records(const char *shape, const FVM_SRecord *list, FVM_SRecord *refs, size_t *count);

/* EN: Borrow sparse updates after validating their mask, schemas and unique indices.
 * RU: Заимствовать обновления после проверки маски, схем и уникальных индексов. */
int styxmanaged_updates(const char *arguments, const void *bytes, size_t size,
    char selected[STYX_SHAPE_MAX + 1], unsigned *mask, FVM_SRecord *updates);


/* EN: Validate a managed argument or result list and borrow its decoded outer record.
 * RU: Проверить список управляемых значений и заимствовать внешнюю запись.
 * Shape is a NUL-terminated list. Ref identities are local to one record.
 * Схема должна быть NUL-строкой. Идентичность ref действует в одной записи. */
int styxmanaged_list_read(unsigned long tag, const char *shape, const void *bytes, size_t size,
    FVM_SRecord *list);


/* EN: Validate encoded children against shape and seal them as one list.
 * RU: Проверить закодированных потомков по схеме и упаковать их в один список.
 * Payload and destination must be disjoint. Failure does not publish used.
 * Payload и приёмник не пересекаются. Ошибка не меняет used. */
int styxmanaged_list_write(unsigned long tag, const char *shape, const void *payload, size_t size,
    void *bytes, size_t capacity, size_t *used);

/* EN: Pack exactly two complete records, result first and updates second.
 * RU: Упаковать ровно две полные записи, результат первым, обновления вторыми. */
int styxcall_reply_write(unsigned long tag, const void *result, size_t result_size,
    const void *updates, size_t updates_size, void *bytes, size_t capacity, size_t *used);


typedef struct StyxReplyField
{
    const void *bytes; /* Borrowed complete encoded field. */
    size_t size; /* Byte extent of that field. */
} StyxReplyField;


/* EN: Borrow the two encoded fields of a complete checked reply, in order.
 * RU: Заимствовать два закодированных поля проверенного ответа по порядку.
 * On failure fields stay unchanged. Callers still validate value types.
 * При отказе fields сохраняется. Типы значений проверяет вызывающий. */
int styxcall_reply_read(unsigned long tag, const void *bytes, size_t size, StyxReplyField fields[2]);
/* EN: Validate borrowed children against the value-list schema.
 * RU: Проверить заимствованные дочерние записи по схеме списка. */
int styxmanaged_payload_valid(const char *shape, const void *bytes, size_t size);
/* EN: Match a single complete, empty nil record.
 * RU: Проверить одну полную запись nil без полезной нагрузки. */
int styxmanaged_is_nil(const void *bytes, size_t size);
#endif
