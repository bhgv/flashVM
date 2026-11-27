/* Copyright (c) 2026 bhgv
 * EN: Private handle ownership shared by OS admission and its methods.
 * RU: Внутреннее владение между открытием и методами OS.
 */
#ifndef FVM_STYXRPC_OS_HANDLES_H
#define FVM_STYXRPC_OS_HANDLES_H
#include "movie_load.h"
#include "flashvm/value.h"

typedef struct OsHandle
{
    FVM_StyxRpcMovies *movies; /* Borrowed playback owner through context teardown. */
    StyxRpcVfsFile *file;      /* Owned regular file or NULL. */
    StyxRpcVfsDir *directory;  /* Owned ordinary directory iterator. */
    struct StyxFid *module;   /* Owned unopened directory of the checked image. */
    uint64_t offset;           /* Confirmed byte cursor. */
    int uncertain;             /* Last failed write may have modified remote bytes. */
    char path[FVM_STYXRPC_MOVIE_PATH]; /* Canonical path including leading slash. */
} OsHandle;


/* EN/RU: Check a four-byte suffix. / Проверить четырёхбайтовое расширение. */
int fvm_os_extension(const char *, const char *);

/* EN/RU: Materialize VFS metadata. / Создать объект метаданных VFS. */
FVM_Result fvm_os_attributes(FVM_Context *, const StyxRpcFileInfo *, const char *, FVM_Value *);

/* EN/RU: Consume handle and create its wrapper. / Принять handle и создать обёртку. */
FVM_Result fvm_os_wrap_handle(FVM_Context *, OsHandle *, int, FVM_Value *);
#endif
