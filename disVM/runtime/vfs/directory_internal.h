/* Copyright (c) 2026 bhgv
 * EN: Native directory projection helpers for exact child bindings.
 * RU: Native helpers directory projection для точных child bindings.
 */
#ifndef DVM_DIRECTORY_INTERNAL_H
#define DVM_DIRECTORY_INTERNAL_H
#include "internal.h"


/* EN/RU: Copied child spelling; no provider pointer or guest ownership.
 * Копия child spelling, без provider pointer и guest ownership. */
typedef struct DirectoryName
{
    char path[DVM_NS_PATH_LIMIT]; /* Absolute exact child mount point. */
} DirectoryName;


/* EN/RU: Capture unique immediate binding names at one revision;1/0, owned output/empty on error.
 * No provider callbacks under lock, bounded stale retries. Caller retains group.
 * Захватить unique immediate bindings одной revision;1/0, свой output/empty при error. */
int dvm_directory_names(DVM_Namespace *group, const char *parent, DirectoryName **names, size_t *count);


/* EN/RU: Borrow direct native base directory for lifetime diagnostics under exclusive handle pin.
 * NULL for other handles/provider/union bases; no I/O, no ownership transfer, never close it.
 * Заимствовать direct native directory для lifetime diagnostics под exclusive pin; NULL иначе. */
FVM_PortDirectory *dvm_directory_native(DVM_VfsHandle *handle);
#endif
