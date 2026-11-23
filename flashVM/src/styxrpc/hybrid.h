/* Copyright (c) 2026 bhgv
 * EN: Private typed userdata access and table publication for FlashVM graph calls.
 * RU: Закрытый доступ к данным таблицы и её публикация для графовых вызовов FlashVM.
 */
#ifndef FLASHVM_STYXRPC_HYBRID_PRIVATE_H
#define FLASHVM_STYXRPC_HYBRID_PRIVATE_H
#include "flashvm/styxrpc_hybrid.h"
#include "styxrpc/hybrid.h"

#ifdef __cplusplus
extern "C" {
#endif


/* [GC:] EN: Release the table payload exactly once when the owner dies.
 * RU: Однократно освободить таблицу при уничтожении владельца. */
void fvm_styxrpc_hybrid_release(void *table);


/* EN: Borrow native storage only from a same-context table owner.
 * RU: Заимствовать хранилище только у таблицы этого контекста. */
StyxRpcHybrid *fvm_styxrpc_hybrid_lookup(FVM_Context *context, FVM_Object *object);


/* EN: Swap a prepared replacement on its verified owner; return the old table in replacement.
 * RU: Подменить подготовленную таблицу проверенного владельца, вернуть старую в replacement. */
void fvm_styxrpc_hybrid_swap(FVM_Object *object, StyxRpcHybrid **replacement);


/* [GC:] EN: Adopt table only on success into a new local object with its AS methods.
 * RU: Только при успехе передать таблицу новому объекту с методами AS. */
FVM_Result fvm_styxrpc_hybrid_adopt(FVM_Context *context, StyxRpcHybrid *table, FVM_Value *out);

#ifdef __cplusplus
}
#endif
#endif
