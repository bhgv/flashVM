/* Copyright (c) 2026 bhgv
 * EN: Startup library dependencies shared by the launch and library services.
 * RU: Зависимости при старте сервисов запуска и библиотек.
 */
#ifndef DVM_TERMINAL_SERVE_LIBRARIES_H
#define DVM_TERMINAL_SERVE_LIBRARIES_H
#include "config.h"
#include "vfs/namespace.h"
#include "mount.h"
#include "caller_styx.h"
#include "mux/bidir.h"
#include <stddef.h>

#define LIBRARY_MOUNT_LIMIT 8U /* Maximum library mounts admitted before startup. */

/* EN: Borrowed startup selection for one independent Styx dependency.
 * RU: Заимствованный выбор одной независимой зависимости Styx. */
typedef struct LibraryMount
{
    const char *descriptor; /* Borrowed argument naming the delegated stream. */
    const char *path; /* Borrowed owner-side absolute mount path. */
    long number; /* Parsed descriptor used to detect duplicates. */
    int bidirectional; /* Use the stock library service with reverse caller VFS. */
} LibraryMount;


/* EN: Owned transport and reverse service of one attempted mount.
 * RU: Свой транспорт и встречный сервер одной попытки маунта. */
typedef struct LibrarySession
{
    PhyBidir *mux; /* Retained through the runtime and reverse worker shutdown. */
    StyxCallerServer *reverse; /* Owned reverse caller service on the same socket. */
    int breaker; /* Shutdown duplicate, valid when mux setup was attempted. */
} LibrarySession;


/* EN: Append validated mount choices and update the borrowed argument index.
 * RU: Добавить проверенные маунты и обновить индекс заимствованных аргументов. */
int dvm_terminal_parse_libraries(LibraryMount *mounts, size_t *count, int argc, char **argv, int *index);


/* EN: Check that the primary socket and all dependency sockets are distinct.
 * RU: Проверить, что основной сокет и все сокеты зависимостей различны. */
int dvm_terminal_distinct_carriers(int primary, const LibraryMount *mounts, size_t count);


/* EN: Install the borrowed mount entries, retaining each attempted session for
 * cleanup. Returns zero at the first failure and always writes the attempted count.
 * RU: Установить заимствованные маунты, сохраняя каждую попытку для очистки.
 * Вернуть ноль при первом отказе, всегда записать количество попыток. */
int dvm_terminal_mount_libraries(DVM_Namespace *group, const LibraryMount *mounts,
    size_t count, LibrarySession *sessions, size_t *opened);


/* EN: Close the indicated number of initialized sessions in reverse order.
 * RU: Закрыть заданное число инициализированных сессий в обратном порядке. */
void dvm_terminal_close_libraries(LibrarySession *sessions, size_t opened);
#endif
