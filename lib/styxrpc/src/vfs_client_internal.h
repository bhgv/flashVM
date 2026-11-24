/* Copyright (c) 2026 bhgv
 * EN: Common public path admission for the hosted client.
 * RU: Общий допуск публичных путей хостового клиента.
 */
#ifndef STYXRPC_VFS_CLIENT_INTERNAL_H
#define STYXRPC_VFS_CLIENT_INTERNAL_H
/* EN: Check a nonempty relative path outside reserved .prog components.
 * RU: Проверить непустой относительный путь вне служебных компонентов .prog. */
int styxrpc_vfs_visible_path(const char *path);
#endif
