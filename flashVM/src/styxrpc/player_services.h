/* Copyright (c) 2026 bhgv
 * EN: Owner state of one paired movie's script session and module publication.
 * RU: Состояние сессии AS и опубликованного образа одного фильма у владельца.
 */
#ifndef FLASHVM_STYXRPC_PLAYER_SERVICES_H
#define FLASHVM_STYXRPC_PLAYER_SERVICES_H
#include "flashvm/styxrpc_session.h"
#include "styxrpc/module_slot.h"

typedef struct FVM_StyxRpcPlayerServices FVM_StyxRpcPlayerServices; /* Opaque state owned by playback. */


/* EN: Install script facilities and publish their cleanup state even on partial failure.
 * RU: Установить службы AS и выдать состояние очистки даже при частичном отказе. */
FVM_Result fvm_styxrpc_player_services(FVM_Context *context, struct StyxClient *client,
    StyxRpcModuleSlot *slot, FVM_StyxRpcPlayerServices **out);


/* EN: Make services current, retiring any old image fids. NULL clears slot.
 * RU: Сделать services текущими и отозвать старые fid. NULL очищает слот. */
FVM_Result fvm_styxrpc_player_activate(StyxRpcModuleSlot *slot, FVM_StyxRpcPlayerServices *services);


/* EN: Close launch fids and unpin the withdrawn image before destroying the context.
 * RU: Закрыть fid запуска и освободить корень отозванного образа до удаления контекста. */
void fvm_styxrpc_player_disarm(FVM_StyxRpcPlayerServices *services);

#endif
