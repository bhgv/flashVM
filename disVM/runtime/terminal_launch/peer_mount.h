/* Copyright (c) 2026 bhgv
 * EN: Shared launcher-side discovery for a bidirectional Styx mount.
 * RU: Общее обнаружение запусков на двустороннем маунте Styx.
 */
#ifndef DVM_TERMINAL_PEER_MOUNT_H
#define DVM_TERMINAL_PEER_MOUNT_H

#include "caller_styx.h"


/* EN: Consume client on every outcome and borrow the reverse caller server.
 * An empty override (entry == NULL) discovers advertised executables. A
 * supplied entry/request/tag selects the legacy explicit route; scoped
 * selects its caller context. Return an owned mount or NULL on failure.
 * RU: Принять client при любом исходе и заимствовать встречный сервер.
 * При entry == NULL обнаружить опубликованные исполняемые файлы.
 * Заданные entry/request/tag явно выбирают прежний маршрут, scoped задаёт
 * контекст вызывающего. Вернуть свой маунт либо NULL при отказе. */
DVM_Provider *dvm_terminal_mount_peer(StyxCallerServer *reverse, StyxClient *client,
    const char *entry, const char *request, unsigned long tag, int scoped);

#endif
