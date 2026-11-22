/* Copyright (c) 2026 bhgv
 * EN: Call-local references and transactional ADT copy-back.
 * RU: Ссылки внутри вызова и транзакционная обратная запись ADT.
 */
#ifndef DVM_STYX_MANAGED_REF_H
#define DVM_STYX_MANAGED_REF_H
#include "managed_value.h"

typedef struct DVM_StyxRefTable
{
    void *objects[STYX_CALL_REF_KEYS]; /* Original argument objects followed by owned traversal objects. */
    size_t count; /* Number of reserved reference indices. */
    size_t limit; /* Maximum result indices, or zero for the standalone limit. */
    int result; /* Share the table through the validated result tree. */
} DVM_StyxRefTable;


/* EN: Remove owned entries starting at first, preserving borrowed identities.
 * RU: Освободить собственные записи начиная с first, сохраняя заимствованные. */
void dvm_styx_ref_release(DVM_StyxRefTable *table, size_t first);


/* EN: Decode an owned local object or retain an earlier alias from table.
 * RU: Декодировать собственный локальный объект либо удержать прежний из table. */
int dvm_styx_ref_read(DVM_StyxRefTable *table, const char *shape, const FVM_SRecord *node, void **result);


/* EN: Encode a rooted object once and use its index for subsequent aliases.
 * RU: Кодировать удерживаемый объект один раз и дальше выдавать его индекс. */
int dvm_styx_ref_write(DVM_StyxRefTable *table, const char *shape, void *object,
    void *bytes, size_t capacity, size_t *used);


/* EN: Encode or decode fields of a nested result object with the shared table.
 * RU: Кодировать или декодировать поля вложенного объекта с общей таблицей. */
int dvm_styx_result_fields_read(DVM_StyxRefTable *table, const char *shape,
    const void *payload, size_t size, DVM_StyxValueFrame *frame);
int dvm_styx_result_fields_write(DVM_StyxRefTable *table, const char *shape, const void *slots,
    size_t size, void *bytes, size_t capacity, size_t *used);


/* EN/RU: Codec for a call-local graph. The caller owns entries appended to table. */
int dvm_styx_graph_write(DVM_StyxRefTable *table, unsigned long tag, const char *shape,
    const void *slots, size_t size, void *bytes, size_t capacity, size_t *used);
int dvm_styx_graph_reply_write(const StyxCallShape *shape, const FVM_SRecord *baseline,
    void *const *identities, size_t count, const void *slots, size_t size, void *bytes, size_t capacity, size_t *used);
int dvm_styx_graph_reply_commit(const StyxCallShape *shape, const FVM_SRecord *baseline,
    DVM_StyxRefTable *refs, void *destination, const void *bytes, size_t size);

/* EN: Gather only borrowed ref slots from the argument frame.
 * RU: Выбрать только заимствованные слоты ref из кадра аргументов. */
int dvm_styx_ref_slots(const char *shape, const void *arguments, char *refs, void **objects);


/* EN: Install the decoded result and updates together. Caller clears both stages.
 * RU: Применить результат и обновления вместе. Вызывающий очищает оба буфера. */
int dvm_styx_ref_commit(const char *refs, void *const *objects, DVM_StyxValueFrame *updates,
    void *destination, DVM_StyxValueFrame *result);
/* EN: Write the mask and snapshots for changed ADTs only.
 * RU: Записать маску и снимки только изменившихся ADT. */
int dvm_styx_ref_updates(const char *arguments, const void *slots, const FVM_SRecord *baseline,
    void *bytes, size_t capacity, size_t *used);
/* EN: Apply validated field deltas and the return value in one commit.
 * RU: Применить проверенные дельты полей и результат одним действием. */
int dvm_styx_ref_delta_commit(const char *arguments, const void *slots, const StyxFieldDelta *changes,
    size_t count, void *destination, DVM_StyxValueFrame *result);
#endif
