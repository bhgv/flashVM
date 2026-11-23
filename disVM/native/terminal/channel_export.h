/* Copyright (c) 2026 bhgv
 * EN: Native return-channel leases and transport fids on a mounted module.
 * RU: Нативные аренды возвращаемых каналов и транспортные fid в каталоге модуля.
 */
#ifndef STYX_CHANNEL_EXPORT_H
#define STYX_CHANNEL_EXPORT_H
#include "provider.h"
#include "cancel.h"
#include "call_record.h"

#define STYX_CHANNEL_EXPORT_FILE ".unibus" /* Existing session, module-scoped channel operations. */

typedef struct StyxChannelExports StyxChannelExports; /* One module's native lease registry. */


/* EN: Allocate an empty owned registry. Destroy it after the module's fids retire.
 * RU: Выделить пустой реестр. Освободить после закрытия fid модуля. */
StyxChannelExports *styxchannel_exports_create(void);
void styxchannel_exports_destroy(StyxChannelExports *exports);


/* EN: Reserve an owned, unpublished control for exactly one leased function fid.
 * RU: Зарезервировать собственное управление для одного арендующего fid вызова. */
DVM_Provider *styxchannel_export_create(StyxChannelExports *exports);


/* EN: Revoke the borrowed control on clunk. Existing operations wake with cancellation.
 * RU: Отозвать заимствованное управление при clunk. Текущие операции отменяются. */
void styxchannel_export_revoke(DVM_Provider *control);


/* EN: Copy the immutable identity or obtain an owned sticky revocation source.
 * RU: Скопировать неизменяемый идентификатор либо получить собственный токен отзыва. */
int styxchannel_export_token(DVM_Provider *control, unsigned char token[STYX_CHANNEL_EXPORT_TOKEN_SIZE]);
DVM_Cancel *styxchannel_export_cancel(DVM_Provider *control);


/* EN: Publish one borrowed native bus, retained until the lease retires.
 * RU: Опубликовать заимствованный нативный канал, удерживая его до отзыва аренды. */
int styxchannel_export_publish(DVM_Provider *control, DVM_Provider *bus);


/* EN: Bind a fresh opened operation to the module registry. Return an owned handle.
 * RU: Связать свежую операцию с реестром модуля и вернуть собственный handle. */
int styxchannel_export_open(StyxChannelExports *exports, DVM_VfsHandle *handle);
#endif
