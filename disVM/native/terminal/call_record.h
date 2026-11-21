/* Copyright (c) 2026 bhgv
 * EN: Ordered typed S-records for a mounted Dis argument list or return.
 * RU: Упорядоченные типизированные S-записи аргументов или результата Dis.
 */
#ifndef STYX_CALL_RECORD_H
#define STYX_CALL_RECORD_H

#include <stddef.h>
#include <stdint.h>

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
    size_t size; /* Exact byte extent of the pointer-free NC64 layout. */
    size_t count; /* Number of scalar leaves. */
    size_t offset[STYX_CALL_VALUES]; /* Byte offsets in schema order. */
    unsigned char width[STYX_CALL_VALUES]; /* Native leaf widths. */
} StyxScalarLayout;


/* EN: Validate a value-only schema and compute its NC64 layout.
 * RU: Проверить схему значений и вычислить раскладку NC64. */
int styxscalar_layout(const char *shape, StyxScalarLayout *layout);


/* EN: Copy fields from an NC64 buffer and widen packed byte leaves.
 * RU: Скопировать поля буфера NC64 и расширить упакованные поля byte. */
void styxscalar_unpack(const StyxScalarLayout *layout, const void *frame, StyxCallValues *values);


/* EN: Stage fields in an NC64 buffer, zeroing layout padding.
 * RU: Записать поля в буфер NC64, обнуляя заполнители раскладки. */
void styxscalar_pack(const StyxScalarLayout *layout, const StyxCallValues *values, void *frame);
#endif
