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
#define STYX_CALL_REF_KEYS 256U /* All values of the wire reference-index byte. */
#define STYX_CALL_LEGACY_REF_KEYS (2U * STYX_CALL_VALUES) /* Older result profiles. */
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


/* EN: Resolve the single element of a borrowed array type to a frame schema and layout.
 * RU: Получить схему и раскладку одного элемента заимствованного типа массива. */
int styxmanaged_array_layout(const char *array, char element[STYX_SHAPE_MAX + 1],
    StyxScalarLayout *layout);

/* EN: Check each array in a validated schema, including arrays in tuple and ref fields.
 * RU: Проверить каждый массив валидированной схемы, включая поля кортежей и ref. */
int styxmanaged_arrays(const char *shape, int result);

/* EN: Extract the fields of a result reference, allowing nested result references.
 * RU: Получить поля ссылки результата, включая вложенные ссылки результата. */
int styxmanaged_result_ref_fields(const char *shape, char fields[STYX_SHAPE_MAX + 1]);


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
#define STYX_MANAGED_LIST_ITEMS 2048U /* Maximum nodes in one typed Limbo list. */
#define STYX_MANAGED_ARRAY_TAG 8UL /* Sequence of nested records of one declared element type. */
#define STYX_MANAGED_NIL_TAG 9UL /* Empty payload for a nil managed leaf. */
#define STYX_MANAGED_REF_TAG 10UL /* Index byte, then fields on first occurrence. */


#define STYX_MANAGED_TABLE_TAG 20UL /* Complete hybrid table, using ordered typed pairs. */
#define STYX_MANAGED_TABLE_PAIR_TAG 21UL /* Big or string key followed by a big value. */
#define STYX_MANAGED_TABLE_PATCH_TAG 22UL /* Ordered removals followed by changed pairs. */
#define STYX_MANAGED_TABLE_REMOVE_TAG 23UL /* Remove one big or string key. */
#define STYX_MANAGED_CHANNEL_TAG 24UL /* Positive four-byte call-local channel key. */
#define STYX_CHANNEL_PROFILE_FILE ".unibus-version" /* Mounted channel capability profile. */
#define STYX_CHANNEL_PROFILE "unibus-channel-1" /* Exact supported channel wire family. */
#define STYX_CHANNEL_STATUS_FILE ".unibus-status" /* Read-only native diagnostic snapshot. */
#define STYX_MANAGED_CHANNEL_MODULE_GRANT_TAG 35UL /* Nested module token and channel key. */
#define STYX_MANAGED_CHANNEL_GRANT_TAG 33UL /* Nested reverse message grant. */
#define STYX_MANAGED_UNBUFFERED_CHANNEL_TAG 34UL /* Nested owner key with rendezvous semantics. */
#define STYX_MANAGED_EXPORTED_CHANNEL_TAG 31UL /* Result-only provider lease token and channel key. */
#define STYX_CHANNEL_EXPORT_TOKEN_SIZE 16U /* Opaque provider export token. */

typedef struct StyxTableKey
{
    int64_t index; /* Nonnegative positional or sparse key if text is false. */
    const unsigned char *bytes; /* Borrowed exact UTF-8 key bytes when text is true. */
    size_t size; /* Byte extent of bytes, allowing embedded zero bytes. */
    int text; /* Distinguish numeric keys from textual keys. */
} StyxTableKey;

/* EN: Validate one table record and optionally visit its checked rows in order.
 * A false visitor result refuses the record; visitors stage unpublished data.
 * RU: Проверить полную запись таблицы, затем при необходимости обойти строки.
 * Ноль от посетителя означает отказ; посетитель меняет только временные данные. */
int styxmanaged_table_visit(const void *bytes, size_t size, int patch,
    int (*visit)(void *, const StyxTableKey *, int, int64_t), void *state);

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


#define STYX_MANAGED_DELTA_TAG 11UL /* Object mask and per-object field records. */

#define STYX_MANAGED_ARG_UPDATE_TAG 18UL /* Tuple position, field mask and selected values. */
#define STYX_MANAGED_POSITION_UPDATES_TAG 19UL /* Sparse entries may use argument positions. */
#define STYX_MANAGED_SPARSE_GRAPH_UPDATES_TAG 17UL /* Original index, field mask and selected values. */
#define STYX_MANAGED_GRAPH_UPDATES_TAG 16UL /* Changed objects, in request index order. */
#define STYX_MANAGED_EDIT_TAG 15UL /* Ordered offset, removal and insertion records. */
#define STYX_MANAGED_SPLICE_TAG 14UL /* Offset, removal extent, and insertion bytes. */
#define STYX_MANAGED_RUN_TAG 13UL /* Ordered array spans, each with offset and length. */
#define STYX_MANAGED_SPAN_TAG 12UL /* Equal-size array payload replacement. */

/* EN: Encode a smaller array patch when both payload extents match.
 * RU: Кодировать компактную замену при совпадении размеров массивов. */
int styxmanaged_span_write(const FVM_SRecord *before, const FVM_SRecord *after,
    void *bytes, size_t capacity, size_t *used);

/* EN: Expand a checked patch against the original array with bounds checking.
 * RU: Восстановить запись по исходному массиву с проверкой границ. */
int styxmanaged_span_expand(const char *type, const FVM_SRecord *before, const FVM_SRecord *patch,
    unsigned char *scratch, size_t capacity, void *bytes, size_t limit, size_t *used);


typedef struct StyxFieldDelta
{
    unsigned object; /* Index within the projected ref arguments. */
    unsigned mask; /* Selected flattened fields of this ADT. */
    char schema[STYX_SHAPE_MAX + 1]; /* Value schema for the selected leaves. */
    FVM_SRecord fields; /* Borrowed sequence of selected field records. */
} StyxFieldDelta;


/* EN: Compare two validated ref snapshots and encode only changed field leaves.
 * RU: Сравнить два проверенных снимка ref и записать только изменившиеся поля. */
int styxmanaged_diff_fields(const char *type, const FVM_SRecord *before, const FVM_SRecord *after,
    void *bytes, size_t capacity, size_t *used);

/* EN: Validate and borrow the deltas against the original argument schema.
 * RU: Проверить дельты по исходной схеме аргументов и заимствовать их. */
int styxmanaged_deltas(const char *arguments, const void *bytes, size_t size,
    StyxFieldDelta changes[STYX_CALL_VALUES], size_t *count);


/* EN: Validate deltas and expand spans using the retained request and output scratch.
 * RU: Проверить дельты и развернуть участки по сохранённому запросу в выходной буфер. */
int styxmanaged_deltas_base(const char *arguments, const void *bytes, size_t size,
    StyxFieldDelta changes[STYX_CALL_VALUES], size_t *count, const FVM_SRecord *baseline,
    unsigned char *output, size_t capacity);


/* EN: Validate reply values with reference indices seeded by the request.
 * RU: Проверить значения ответа с индексами ссылок из исходного запроса. */
int styxmanaged_payload_linked(const char *shape, const void *bytes, size_t size,
    const char *argument_shape, const FVM_SRecord *arguments);

/* EN: Borrow a complete result record, rejecting references to unknown objects.
 * RU: Заимствовать полную запись результата и отклонить неизвестные ссылки. */
int styxmanaged_result_read(const char *shape, const void *bytes, size_t size,
    const char *argument_shape, const FVM_SRecord *arguments, FVM_SRecord *result);


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
typedef struct StyxRefDefinition
{
    const char *shape; /* Borrowed start of an ADT schema in the input shape. */
    FVM_SRecord record; /* Borrowed defining record including its index byte. */
} StyxRefDefinition;

/* EN: Index definitions in a checked value graph by wire identity. Limit is the
 * number of available entries (1–256). Discard entries on failure.
 * RU: Собрать определения проверенного графа по сетевым индексам. Limit задаёт
 * число доступных записей (1–256). При ошибке записи использовать нельзя. */
int styxmanaged_ref_definitions(const char *shape, const FVM_SRecord *list, int nested,
    StyxRefDefinition *definitions, size_t limit, size_t *count);

/* EN: Validate borrowed children against the value-list schema.
 * RU: Проверить заимствованные дочерние записи по схеме списка. */
int styxmanaged_payload_valid(const char *shape, const void *bytes, size_t size);
/* EN: Validate the argument payload according to its negotiated request tag.
 * RU: Проверить аргументы по согласованному тегу запроса. */
int styxmanaged_request_payload(const char *shape, const FVM_SRecord *list);


/* EN: Validate a message graph and require admission for each non-nil channel.
 * The optional application data is borrowed through the synchronous callback.
 * RU: Проверить граф сообщения и полномочие каждого канала, отличного от nil.
 * Данные context заимствуются на время синхронного обратного вызова. */
int styxmanaged_channel_payload(const char *shape, const FVM_SRecord *list,
    int (*admit)(const void *, const char *, uint32_t), const void *context);

/* EN: Validate a channel value and opt into reverse grant records.
 * RU: Проверить типизированное сообщение с необязательными встречными полномочиями. */
int styxmanaged_channel_value(const char *shape, const FVM_SRecord *list,
    int (*admit)(const void *, const char *, uint32_t), const void *context, int grants);

/* EN: Validate the graph and delegate each non-nil channel leaf to a synchronous visitor.
 * The visitor must validate the tag and payload and stage any changes until success.
 * RU: Проверить граф, передав каждый ненулевой канал синхронному обработчику.
 * Обработчик проверяет тег и данные и применяет накопленные изменения после успеха. */
int styxmanaged_channel_routes(const char *shape, const FVM_SRecord *list,
    int (*route)(void *, const char *, const FVM_SRecord *), void *context);

typedef struct StyxGraphPositions
{
    unsigned short argument[STYX_CALL_REF_KEYS]; /* Earliest direct position plus one, zero for nested-only refs. */
    unsigned short reference[STYX_SHAPE_MAX]; /* Identity plus one at each canonical direct position. */
} StyxGraphPositions;

/* EN: Build the direct argument map for a validated positional request.
 * For other dialects leave positions disabled. Returns zero on inconsistent input.
 * RU: Построить карту прямых параметров проверенного запроса с позиционным тегом.
 * Для других диалектов карта пуста. Ноль означает несогласованные данные. */
int styxmanaged_graph_positions(const char *shape, const FVM_SRecord *arguments, StyxGraphPositions *positions);

/* EN: Resolve a reply selector to an original identity. Reject invalid selectors
 * and noncanonical aliases before a caller can index the reference table.
 * RU: Найти исходную ссылку по селектору ответа. Отклонить неверные и повторные
 * позиции до обращения к таблице ссылок. */
int styxmanaged_graph_update_index(const StyxGraphPositions *positions, const FVM_SRecord *update,
    size_t originals, size_t *index);

/* EN: Select the reply list tag belonging to the checked graph dialect.
 * RU: Выбрать тег списка ответа для проверенного диалекта графа. */
unsigned long styxmanaged_graph_updates_tag(unsigned long request);

/* EN: Validate both graph-reply positions using one identity namespace.
 * RU: Проверить обе части ответа с общей таблицей ссылок. */
/* EN: Validate both positions using argument identities, visiting exported channel leaves.
 * RU: Проверить обе позиции с идентичностями аргументов, обходя каналы результата. */
int styxmanaged_graph_reply_routes(const StyxCallShape *shape, const FVM_SRecord *arguments,
    const StyxReplyField fields[2], int (*route)(void *, const char *, const FVM_SRecord *), void *context);


int styxmanaged_graph_reply(const StyxCallShape *shape, const FVM_SRecord *arguments,
    const StyxReplyField fields[2]);
/* EN: Compare checked ADT fields by value, comparing ref records by index.
 * RU: Сравнить проверенные поля ADT по значению, а записи ref по индексу. */
int styxmanaged_graph_mask(const FVM_SRecord *before, const FVM_SRecord *after, unsigned *mask);

/* EN: Encode only changed leaves of two checked snapshots. Compare refs by index.
 * RU: Закодировать только изменившиеся поля двух снимков. Ссылки сравниваются по индексу. */
int styxmanaged_graph_diff(const char *type, const FVM_SRecord *before, const FVM_SRecord *after,
    void *output, size_t capacity, size_t *used);
/* EN: Read the mask and derive the flattened schema of the selected fields.
 * RU: Прочитать маску и вывести развёрнутую схему выбранных полей. */
int styxmanaged_graph_fields(const char *type, const FVM_SRecord *update, StyxFieldDelta *change);

/* EN: Match a single complete, empty nil record.
 * RU: Проверить одну полную запись nil без полезной нагрузки. */
int styxmanaged_is_nil(const void *bytes, size_t size);
#endif
