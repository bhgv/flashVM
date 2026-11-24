/* Copyright (c) 2026 bhgv
 * EN: Project an attached Styx tree into FlashVM files and live script variables.
 * RU: Подключение дерева Styx к файлам FlashVM и живым переменным сценария.
 */
#ifndef FLASHVM_STYXRPC_FILESYSTEM_H
#define FLASHVM_STYXRPC_FILESYSTEM_H
#include "filesystem.h"

#ifdef __cplusplus
extern "C" {
#endif

struct StyxClient; /* Already attached Styx client selected by the host. */


/* EN: Mount session as ordinary files under path. Use / for exclusive paired mode in a fresh context.
 * Borrow session until unmount or context teardown.
 * Open fids independently retain its transport. The host can disconnect it explicitly.
 * RU: Смонтировать session как обычные файлы под path. Для парного режима укажите / в новом контексте.
 * Заимствовать session до снятия монтирования
 * или освобождения контекста. Открытые fid удерживают транспорт независимо. Хост может отключить его. */
FVM_Result fvm_styxrpc_filesystem_mount(FVM_Context *context, const char *path, struct StyxClient *session);


/* [GC:] EN: Bind one new property to an existing native int variable path on session.
 * The property retains an exact fid. writable is 0 or 1; no assignment copies a proxy image.
 * RU: Привязать новое свойство к пути существующей переменной int на session.
 * Свойство удерживает точный fid. writable равен 0 или 1. Присваивание обращается к владельцу. */
FVM_Result fvm_styxrpc_int_variable_bind(FVM_Context *context, FVM_Object *object, const char *name,
    struct StyxClient *session, const char *path, int writable);

#ifdef __cplusplus
}
#endif
#endif
