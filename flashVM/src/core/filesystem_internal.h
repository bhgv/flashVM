/* Copyright (c) 2026 bhgv
 * EN: Owner context path dispatch shared by file objects and file channels.
 * RU: Привязка путей контекста для файловых объектов и файловых каналов.
 */
#ifndef FVM_FILESYSTEM_INTERNAL_H
#define FVM_FILESYSTEM_INTERNAL_H
#include "flashvm/filesystem.h"

#ifdef __cplusplus
extern "C" {
#endif

struct FVM_PortFile; /* Owned file descriptor with a native or mounted backend. */

/* EN: Open path through context and transfer a file cursor on success.
 * RU: Открыть path через context и передать файловый курсор при успехе. */
FVM_Result fvm_filesystem_open(FVM_Context *context, const char *path, const char *mode, struct FVM_PortFile **file);


/* EN: Copy stat from the selected path provider into caller-owned info.
 * RU: Скопировать stat выбранного провайдера пути в info вызывающего. */
FVM_Result fvm_filesystem_stat(FVM_Context *context, const char *path, FVM_FsInfo *info);


/* EN: Visit a bounded directory page with a caller-owned callback state.
 * RU: Посетить ограниченную страницу каталога с состоянием колбэка вызывающего. */
FVM_Result fvm_filesystem_list(FVM_Context *context, const char *path, size_t skip, size_t limit,
    FVM_FsVisit visit, void *state);


/* EN: Drop context-owned mount descriptors after finalizing script objects.
 * RU: Освободить монтирования контекста после финализации объектов сценария. */
void fvm_filesystem_cleanup(FVM_Context *context);

#ifdef __cplusplus
}
#endif
#endif
