/* Copyright (c) 2026 bhgv
 *
 * EN: Shared confirmation of a rooted cycle snapshot by Fast GC.
 * RU: Параллельное подтверждение удерживаемого снимка цикла в Fast GC.
 */
#ifndef DVM_GC_GRAPH_CONFIRMATION_H
#define DVM_GC_GRAPH_CONFIRMATION_H

#include "../common/graph_snapshot.h"
#include "disvm/native_world.h"

/* EN: Validate group under cells in world and return optional final_epoch and watched.
 * RU: Сверить group под cells в world и при необходимости вернуть final_epoch и watched. */
DVM_CellResult dvm_gc_fast_confirm_graph(const RecordGroup *group, DVM_CellDomain *cells,
    DVM_NativeWorld *world, unsigned long *final_epoch, int *watched);

#endif
