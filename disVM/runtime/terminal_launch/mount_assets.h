/* Copyright (c) 2026 bhgv
 * EN: Install an independent read-only import base in a new mounted namespace.
 * RU: Установить независимую базу импорта только для чтения в новой VFS.
 */
#ifndef DVM_TERMINAL_MOUNT_ASSETS_H
#define DVM_TERMINAL_MOUNT_ASSETS_H
#include "../vfs/namespace.h"

/* EN: Pin and expose absolute package under /ide-assets before group publication.
 * RU: Закрепить и выставить абсолютный package под /ide-assets до публикации группы. */
int dvm_terminal_mount_assets(DVM_Namespace *group, const char *package);
#endif
