/* Copyright (c) 2026 bhgv
 * EN: Private graph capture policy for projecting selected live accessors to data.
 * RU: Внутренняя политика записи выбранных живых аксессоров в граф как данных.
 */
#ifndef FVM_GRAPH_SLOT_READER_HPP
#define FVM_GRAPH_SLOT_READER_HPP
#include "flashvm/value.h"

struct FVM_AccessorEntry;


/* [GC:] EN: Borrow the accessor and owner for one read. FVM_NOT_FOUND keeps the
 * existing graph encoding. Return other failures to abort. A reader must retain
 * values it returns, and must not run script or mutate the source graph.
 * RU: Заимствовать аксессор и владельца на одно чтение. FVM_NOT_FOUND оставляет
 * существующее кодирование, другие ошибки прекращают запись. Читатель удерживает
 * результаты и не выполняет сценарий или изменение исходного графа. */
typedef FVM_Result (*FVM_GraphSlotRead)(FVM_Context *, FVM_Object *,
    const FVM_AccessorEntry &, void *, FVM_Value *);


struct FVM_GraphSlotReader
{
    FVM_GraphSlotRead read; /* Optional projection called under the native GC barrier. */
    void *state;           /* Borrowed reader state valid until serialization finishes. */
};


/* [ASINC] [GC:] EN: Serialize value with an optional reader for known live slots.
 * Caller owns data on success and releases it with fvm_serialized_free. On failure
 * output is empty. Use on the owning context thread. The reader may block on I/O.
 * RU: Сериализовать value с необязательным читателем известных живых слотов.
 * При успехе data освобождается через fvm_serialized_free, при ошибке выход пуст.
 * Вызов идёт в потоке контекста. Читатель может ждать ввода-вывода. */
FVM_Result fvm_serialize_graph_slots(FVM_Context *context, const FVM_Value *value,
    unsigned char **data, size_t *size, const FVM_GraphSlotReader *reader);
#endif
