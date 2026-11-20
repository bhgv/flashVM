/* Copyright (c) 2026 bhgv
 * EN: Project authorized ordered record files into a prepared launch.
 * RU: Публикация выданных файлов ordered-записей в подготовленном запуске.
 */
#ifndef STYX_ORDERED_LAUNCH_FILES_H
#define STYX_ORDERED_LAUNCH_FILES_H
#include "files.h"
#include "broker/attach.h"

#define STYX_LAUNCH_FDS_MAX 32U /* Maximum explicit open-file aliases per launch. */

typedef struct StyxLaunchFile
{
    const char *path;    /* Borrowed canonical absolute guest path. */
    unsigned long tag;  /* Application tag already granted in broker options. */
    unsigned access;    /* Nonempty READ/WRITE subset. */
} StyxLaunchFile;


/* EN/RU: Select an exact file under the accepted terminal root and its guest alias.
 * Выбрать точный файл в принятом корне терминала и его имя у гостя. */
typedef struct StyxLaunchFd
{
    const char *path;    /* Borrowed canonical absolute guest path. */
    const char *node;    /* Borrowed canonical relative terminal filename. */
    unsigned access;    /* Nonempty READ/WRITE subset. */
} StyxLaunchFd;


/* EN: Borrow client, exclusive handoff, options and mappings through return.
 * Attach the ordered broker to the handoff's exact terminal grant. Expose only
 * the mapped files in its terminal VFS layer. Return the owned file set, which
 * outlives the launch and is freed after guest FDs and namespace retire.
 * On failure return NULL with handoff unchanged. Native setup may block.
 * RU: Заимствовать client, эксклюзивный handoff, options и mappings до возврата.
 * Подключить брокер к исходному terminal grant. В слое VFS выдать только
 * перечисленные файлы. Вернуть свой file set до завершения namespace и FD.
 * При отказе вернуть NULL, сохранив handoff. Подготовка может блокировать. */
StyxOrderedFiles *styxorderedlaunchfiles(StyxClient *client, DVM_LaunchHandoff *handoff,
    const StyxOrderedOptions *options, const StyxLaunchFile *mappings, size_t count);


/* EN/RU: Also project explicitly delegated file nodes from the same captured terminal.
 * Borrow fd mappings (up to 32) through return; capture and stat do not open them.
 * All aliases are prepared before replacement. Guest Sys->open obtains its own FD.
 * Дополнительно выдать указанные файлы принятого корня терминала. Заимствовать
 * до 32 mappings; capture и stat не открывают файлы. При отказе handoff цел.
 * Sys->open у гостя получает собственный FD. */
StyxOrderedFiles *styxorderedlaunchfileswithfds(StyxClient *client, DVM_LaunchHandoff *handoff,
    const StyxOrderedOptions *options, const StyxLaunchFile *mappings, size_t count,
    const StyxLaunchFd *fds, size_t fd_count);
#endif
