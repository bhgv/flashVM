/* Copyright (c) 2026 bhgv
 * EN: Private indexed access to fixed arrays of live integer cells.
 * RU: Внутренний индексированный доступ к фиксированным массивам ячеек.
 */
#ifndef DVM_CAP_ARRAY_INDEX_H
#define DVM_CAP_ARRAY_INDEX_H
#include "export.h"

/* EN: Map an indexed selector and value pair, retaining the borrowed cells.
 * RU: Отобразить выбор индекса и значение, удерживая заимствованные ячейки. */
int dvm_intarray_map_index(DVM_View *view, DVM_Provider *const *cells, unsigned count);
#endif
