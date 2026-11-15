/* Copyright (c) 2026 bhgv
 * EN: Private event-file construction and source authorization helpers.
 * RU: Private event-file construction и source authorization helpers.
 */
#ifndef DVM_VFS_EVENTFS_INTERNAL_H
#define DVM_VFS_EVENTFS_INTERNAL_H
#include "eventfs.h"


/* EN/RU: Borrow source and check its current root read authority; no metadata.
 * Заимствовать source, проверить current root READ authority; без metadata. */
FVM_Result dvm_eventfs_allowed(DVM_Provider *source);


/* EN/RU: Build fixed projection metadata for known path; enforce current authority.
 * Построить metadata known path, проверив current authority; output on success. */
FVM_Result dvm_eventfs_info(DVM_Provider *source, const char *path, FVM_FileInfo *output);


/* EN/RU: Open events in rw mode into empty output, retaining source per handle.
 * Открыть events в rw в empty output; удержать source на handle. Native-only. */
FVM_Result dvm_eventfile_open(DVM_Provider *source, const char *mode, DVM_VfsHandle *output);
#endif
