/* Copyright (c) 2026 bhgv
 * EN: One replaceable loadable image beside stable playback controls.
 * RU: Один заменяемый загружаемый образ рядом с постоянными файлами управления.
 */
#ifndef STYXRPC_MODULE_SLOT_H
#define STYXRPC_MODULE_SLOT_H
#include "styxrpc/module.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct StyxRpcModuleSlot StyxRpcModuleSlot; /* Owner-thread publication and open-fid list. */


/* EN: Create an empty slot borrowing control through destruction, or return NULL.
 * RU: Создать пустой слот, заимствующий control до уничтожения, иначе вернуть NULL. */
StyxRpcModuleSlot *styxrpc_module_slot_create(StyxRpcControl *control);


/* EN: Replace the borrowed image, or pass NULL to withdraw. Retire old open files before return.
 * Call on the server owner between requests or from a control callback, outside an image callback.
 * INVALID leaves the current image unchanged. Keep the new image alive until withdrawal or destroy.
 * RU: Заменить заимствованный образ. NULL отзывает публикацию. Старые открытые файлы закрываются.
 * Вызывать у владельца между запросами или из колбэка управления, вне методов образа.
 * INVALID сохраняет прежний образ. Новый образ должен жить до отзыва или уничтожения слота. */
StyxRpcStatus styxrpc_module_slot_set(StyxRpcModuleSlot *slot, StyxRpcModule *module);


/* EN: Release slot after the server has closed its fids. NULL is allowed.
 * RU: Освободить слот после закрытия fid сервером. NULL допустим. */
void styxrpc_module_slot_destroy(StyxRpcModuleSlot *slot);

extern const StyxRpcTree styxrpc_module_slot_tree; /* Tree context is a live StyxRpcModuleSlot. */

#ifdef __cplusplus
}
#endif
#endif
