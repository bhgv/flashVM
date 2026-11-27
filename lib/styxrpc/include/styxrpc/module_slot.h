/* Copyright (c) 2026 bhgv
 * EN: One replaceable loadable image beside stable playback controls.
 * RU: Один заменяемый загружаемый образ рядом с постоянными файлами управления.
 */
#ifndef STYXRPC_MODULE_SLOT_H
#define STYXRPC_MODULE_SLOT_H
#include "styxrpc/module.h"
#include "styxrpc/program.h"
#include "styxrpc/named_channel.h"
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


/* EN: Release slot after the server has closed its fids, outside node callbacks. NULL is allowed.
 * RU: Освободить слот после закрытия fid сервером, вне колбэков узлов. NULL допустим. */
void styxrpc_module_slot_destroy(StyxRpcModuleSlot *slot);


/* EN: Create an owned channel registry that reserves names in slot. Keep slot
 * alive until the registry and all of its endpoint owners have retired.
 * RU: Создать свой реестр каналов с резервами в slot. Сохранять slot живым
 * до завершения реестра и всех владельцев его участников. */
StyxRpcNamedRegistry *styxrpc_module_slot_named_registry(StyxRpcModuleSlot *slot);

/* EN: Attach an initialized program registry before serving the slot. Borrow it
 * through server and slot destruction. The dispatcher owns registry access;
 * mutate registrations only on that owner between requests. The .prog root
 * is directly traversable for RPC and omitted from root enumeration.
 * Return 1 on first attachment or 0 without a change.
 * RU: Подключить инициализированный реестр программ до обслуживания слота.
 * Заимствовать до уничтожения сервера и слота. Реестром владеет диспетчер,
 * менять регистрации можно только у владельца между запросами. Корень
 * .prog доступен по имени для RPC и скрыт из списка корня.
 * Вернуть 1 при первом подключении или 0 без изменений. */
int styxrpc_module_slot_programs(StyxRpcModuleSlot *slot, StyxRpcPrograms *programs);


extern const StyxRpcTree styxrpc_module_slot_tree; /* Tree context is a live StyxRpcModuleSlot. */

#ifdef __cplusplus
}
#endif
#endif
