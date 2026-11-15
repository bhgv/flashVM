/* Copyright (c) 2026 bhgv
 * EN: Explicit host-mounted event files, governed by source read authority.
 * RU: Явно монтируемые host event files под source read authority.
 */
#ifndef DVM_VFS_EVENTFS_H
#define DVM_VFS_EVENTFS_H
#include "provider.h"
#define DVM_EVENT_FRAME_BYTES 128U /* Maximum single textual event/idle frame. */
#define DVM_EVENT_COMMAND_BYTES 32U /* Maximum atomic mask/ack control write. */
#define DVM_EVENT_TOKEN_HEX 16U /* Fixed-width opaque unsigned 64-bit ticket/session IDs. */


/* EN/RU: Create an owned trusted projection factory containing '.', 'events'.
 * Retains source on success; failure leaves it unchanged. Host installs it using
 * ordinary namespace mount API within an authorized source-factory visibility
 * boundary. Every operation requires source.access('.', READ); absent callback
 * refuses. No global registry or guest mount/admin capability. Native-only.
 * Создать свою projection factory; при успехе удержать source. Host монтирует её
 * в разрешённом visibility boundary source. Каждая операция требует source READ;
 * без access callback — отказ. Без global registry и guest admin capability. */
DVM_Provider *dvm_provider_eventfs(DVM_Provider *source);
#endif
