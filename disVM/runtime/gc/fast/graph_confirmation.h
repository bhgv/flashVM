/* Copyright (c) 2026 bhgv
 *
 * EN: Shared confirmation of a rooted cycle snapshot by Fast GC.
 * RU: Параллельное подтверждение удерживаемого снимка цикла в Fast GC.
 */
#ifndef DVM_GC_GRAPH_CONFIRMATION_H
#define DVM_GC_GRAPH_CONFIRMATION_H

#include "../common/graph_snapshot.h"
#include "disvm/native_world.h"

/* [GC:] EN: Validate caller-owned rooted group under cells in borrowed world.
 * Return ACQUIRED after atomic claim, BUSY for transient guards/native-vector
 * storage pressure, or REFUSED for stale ownership/fields or hard claim failure.
 * final_epoch optionally receives the reader cutoff; native owners require it.
 * reserved reports a successful reserved-field claim. Release all field
 * reservations before return; allocation BUSY preserves every owner/root.
 * RU: Сверить удерживаемый group вызывающего под cells в заимствованном world.
 * Вернуть ACQUIRED после захвата, BUSY при занятых guards/нехватке native-вектора,
 * REFUSED при устаревших полях/владении или жёстком отказе. final_epoch принимает
 * cutoff и обязателен для native-владельцев; reserved сообщает о захвате под
 * резервацией. До возврата снять резервации; BUSY памяти сохраняет владельцев. */
DVM_CellResult dvm_gc_fast_confirm_graph(const RecordGroup *group, DVM_CellDomain *cells,
    DVM_NativeWorld *world, unsigned long *final_epoch, int *reserved);

#endif
