/* Copyright (c) 2026 bhgv
 * EN: A portable offer produced from a linked export on the owner VM.
 * RU: Переносимое предложение из связанного экспорта VM владельца.
 */
#ifndef STYX_LOAD_PROFILE_H
#define STYX_LOAD_PROFILE_H

#include <stddef.h>
#include <stdint.h>

#define STYX_SHAPE_MAX 48U /* Bytes of one shape excluding NUL. */
#define STYX_SHAPED_OFFER_MAX 192U /* Capacity of a complete typed profile record. */
#define STYX_DYNAMIC_CELLS 4U /* Maximum flattened arguments or result cells. */
#define STYX_DYNAMIC_REQUEST_TAG 89UL /* Shape-driven request. */
#define STYX_DYNAMIC_REPLY_TAG 90UL /* Shape-driven successful reply. */
#define STYX_MANAGED_REQUEST_TAG 100UL /* Typed managed argument list. */
#define STYX_MANAGED_DELTA_REQUEST_TAG 103UL /* Opt in to field deltas on this fid. */
#define STYX_MANAGED_SPAN_REQUEST_TAG 104UL /* Opt in to array spans. */
#define STYX_MANAGED_RUN_REQUEST_TAG 105UL /* Opt in to multiple array spans. */
#define STYX_MANAGED_RUN_VERSION 2U /* Length of .ref-fields for multiple spans. */
#define STYX_MANAGED_SPAN_VERSION 1U /* Length of the .ref-fields capability file. */
#define STYX_MANAGED_DELTA_FILE ".ref-fields" /* Owner supports optional field deltas. */
#define STYX_MANAGED_REPLY_TAG 101UL /* Result and updates envelope. */
#define STYX_SHAPED_OFFER_TAG 88UL /* Ordinary export signature and resolved call types. */


/* EN: Select a supported unscoped call profile for the compiled signature.
 * RU: Выбрать профиль вызова без PID по скомпилированной сигнатуре. */
unsigned long styxloadedrequest(uint32_t signature);


/* EN: Encode the linked name/signature into borrowed bytes up to capacity.
 * RU: Закодировать связанное имя и сигнатуру в bytes объёмом capacity. */
int styxloadedoffer(const char *name, uint32_t signature, void *bytes, size_t capacity, size_t *used);


/* EN: Verify a complete export offer for name, and return its request tag.
 * RU: Проверить полное предложение для name и вернуть тег запроса. */
unsigned long styxloadedaccept(const char *name, const void *bytes, size_t size);

typedef struct StyxCallShape
{
    char arguments[STYX_SHAPE_MAX + 1]; /* Ordered list of argument types. */
    char result[STYX_SHAPE_MAX + 1]; /* List containing zero or one return type. */
} StyxCallShape;


/* EN: Verify the grammar and bounds of both NUL-terminated lists in shape.
 * RU: Проверить грамматику и границы двух NUL-терминированных списков shape. */
int styxshapevalid(const StyxCallShape *shape);


/* EN: Return the end of one value type in a borrowed NUL-terminated schema.
 * RU: Вернуть конец одного типа значения в заимствованной NUL-схеме. */
const char *styxshape_end(const char *type);


/* EN: Count the scalar cells and prove both lists fit the generic call codec.
 * RU: Посчитать скалярные ячейки и проверить вместимость обеих записей. */
int styxshapecells(const StyxCallShape *shape, size_t *arguments, size_t *result);


/* EN: Select the existing ref wire format for a validated compiled reference layout.
 * RU: Выбрать существующий формат ref для проверенной компилированной раскладки.
 * Return zero on refusal; results receives the number of scalar return cells.
 * При отказе вернуть ноль; results получает число скалярных ячеек результата. */
unsigned long styxshaperef(const StyxCallShape *shape, size_t *results);


/* EN: Project ref leaves from a validated argument schema in slot order.
 * RU: Выделить ref-поля проверенной схемы аргументов в порядке слотов. */
int styxmanaged_refs(const char *shape, char refs[STYX_SHAPE_MAX + 1], unsigned *mask);


/* EN: Choose a wire tag for a shaped export. Existing profiles retain their tag.
 * RU: Выбрать тег по типам экспорта. Прежние профили сохраняют свои теги. */
unsigned long styxshaperequest(uint32_t signature, const StyxCallShape *shape);


/* EN: Read a complete owner offer and optionally copy its validated schema.
 * RU: Прочитать полное предложение владельца и скопировать его схему. */
unsigned long styxloadedacceptshape(const char *name, const void *bytes, size_t size, StyxCallShape *shape);


/* EN: Encode a checked shape plus the existing compiled identity into bytes.
 * RU: Записать проверенную схему и имеющуюся скомпилированную сигнатуру в bytes. */
int styxshapedoffer(const char *name, uint32_t signature, const StyxCallShape *shape,
    void *bytes, size_t capacity, size_t *used);


/* EN: Validate a complete shaped payload and return its wired request tag.
 * name and signature outputs are optional. A refused record changes no outputs.
 * RU: Проверить полную полезную нагрузку и вернуть тег запроса. name и signature
 * необязательны. При отказе выходные значения сохраняются. */
unsigned long styxshapedpayload(const void *payload, size_t length, char *name, uint32_t *signature, StyxCallShape *shape);

#endif
