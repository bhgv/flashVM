/* Copyright (c) 2026 bhgv
 * EN: Admission of a complete terminal frame on a configured launch route.
 * RU: Приём полного терминального кадра на настроенном маршруте запуска.
 */
#ifndef STYX_TERMINAL_LAUNCH_FRAME_H
#define STYX_TERMINAL_LAUNCH_FRAME_H
#include "../launch/request.h"


/* EN: Borrow one complete encoded S-record and the host's authorized route.
 * Consume one terminal reference on every outcome. Return an owned ticket or
 * NULL. Reject malformed, truncated and concatenated records before Styx I/O.
 * Valid requests may block while capturing remote metadata.
 * RU: Заимствовать одну полную S-запись и разрешённый host маршрут.
 * Поглотить ссылку terminal при любом исходе. Вернуть ticket либо NULL.
 * Повреждённые, неполные и склеенные записи отклоняются до Styx I/O.
 * Допущенный запрос может ждать чтения удалённых метаданных. */
DVM_LaunchTicket *styxterminallaunchframe(StyxClient *terminal, const DVM_LaunchSpec *spec,
    unsigned access, const StyxTerminalPaths *paths, DVM_LaunchEndpoint *endpoint,
    unsigned long tag, const void *bytes, size_t size);
#endif
