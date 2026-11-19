/* Copyright (c) 2026 bhgv
 * EN: Hosted VM traversal-domain initialization before native provider publication.
 * RU: Инициализация traversal domain hosted VM до публикации native providers.
 */
#ifndef DVM_VFS_WALK_BOOT_H
#define DVM_VFS_WALK_BOOT_H


/* EN/RU: Preserve an explicitly configured domain or obtain a fresh 128-bit descriptive VM instance ID;1/0.
 * Startup controller only, before guests/providers; native entropy I/O may block, never under managed admission.
 * IDs are comparison data and confer no authority. Repeated startup keeps this process's published domain.
 * Сохранить explicit domain либо получить fresh 128-bit VM instance ID;1/0. Только startup controller до guests.
 * Native entropy I/O может ждать; IDs не предоставляют authority, повторный startup сохраняет domain процесса.
 */
int dvm_vfs_walk_initialize(void);
#endif
