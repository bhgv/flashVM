/* Copyright (c) 2026 bhgv
 * EN: Private conversion of value-only managed RPC argument/result lists.
 * RU: Внутреннее преобразование списков управляемого RPC по значению.
 */
#ifndef FVM_STYXRPC_MANAGED_VALUES_H
#define FVM_STYXRPC_MANAGED_VALUES_H
#include "managed_refs.h"

#define FVM_STYXRPC_LIST_ITEMS 48U /* Max top-level values in a 48-byte shape. */
#define FVM_STYXRPC_ARRAY_ITEMS 2048U /* Local per-array conversion limit. */


/* EN: Find the end of a verified type token including its compound fields.
 * RU: Найти конец проверенного токена вместе с составными полями. */
const char *fvm_styxrpc_type_end(const char *type);


/* EN: Verify supported bounded lists and a void or single by-value result.
 * RU: Проверить ограниченные схемы значений и пустой либо одиночный результат. */
int fvm_styxrpc_values_supported(const StyxRpcSchema *schema);


/* [GC:] EN: Read owned AS values and root them through the caller's scope.
 * RU: Читать собственные значения AS и удерживать их в области корней вызывающего. */
int fvm_styxrpc_values_decode(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxRefs *refs, const char *shape,
    const FVM_SRecord *record, FVM_Value *values, unsigned capacity, unsigned *count);


/* [GC:] EN: Convert borrowed AS values to bounded wire children; root getter results.
 * RU: Кодировать заимствованные значения с удержанием результатов геттеров. */
int fvm_styxrpc_values_encode(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxRefs *refs, const char *shape,
    const FVM_Value *values, unsigned count, unsigned char *bytes, size_t capacity, size_t *used);
#endif
