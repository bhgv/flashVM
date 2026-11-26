/* Copyright (c) 2026 bhgv
 * EN: Deferred SWF navigation for a playback owner connected to Dis VFS.
 * RU: Отложенная загрузка SWF плеером из подключённой VFS Dis.
 */
#ifndef FVM_STYXRPC_MOVIE_LOAD_H
#define FVM_STYXRPC_MOVIE_LOAD_H
#include "flashvm/flashvm.h"
#include "flashvm/value.h"
#include "styxrpc/vfs_client.h"

#define FVM_STYXRPC_MOVIE_QUEUE 16U /* Maximum outstanding requests per playback owner. */
#define FVM_STYXRPC_MOVIE_PATH (STYXRPC_VFS_PATH_MAX + 2U) /* Absolute path, slash and NUL included. */

typedef struct FVM_StyxRpcMovieRequest
{
    FVM_Object *target;                  /* [GC:] Exact receiver pinned until delivery. */
    char path[FVM_STYXRPC_MOVIE_PATH];   /* Owned canonical path on the attached session. */
} FVM_StyxRpcMovieRequest;

typedef struct FVM_StyxRpcMovies
{
    FVM_Context *context;                /* Borrowed owner. */
    FVM_Object *root;                    /* [GC:] Borrowed playback root pin. */
    struct StyxClient *session;         /* Borrowed attached Dis root. */
    size_t limit;                         /* Maximum wire bytes per child. */
    size_t pending;                       /* Number of queued requests. */
    unsigned long loaded;                /* Complete committed movie replacements since upload. */
    unsigned long failed;                /* Rejected or failed requests since upload. */
    FVM_Result last;                     /* Most recent individual rejection or completion. */
    FVM_StyxRpcMovieRequest queue[FVM_STYXRPC_MOVIE_QUEUE]; /* FIFO in call order. */
} FVM_StyxRpcMovies;


/* [GC:] EN: Bind AVM1 movie requests to this owner before the first timeline activation.
 * RU: Подключить запросы AVM1 к владельцу до первого выполнения кадра. */
FVM_Result fvm_styxrpc_movies_bind(FVM_StyxRpcMovies *movies);


/* [GC:] EN: Resolve one counted native reference and retain its destination at admission.
 * RU: Разрешить ссылку с длиной и удержать назначение при приёме. */
FVM_Result fvm_styxrpc_movie_queue(FVM_StyxRpcMovies *movies, FVM_Object *origin, FVM_Object *target,
    const char *reference, size_t length);


/* EN: Record one rejected request without poisoning the calling timeline.
 * RU: Учесть отклонённый запрос без остановки вызвавшего кадра. */
FVM_Result fvm_styxrpc_movie_reject(FVM_StyxRpcMovies *movies, FVM_Result result);


/* EN: Read a complete asset from one borrowed fid independently of the script cursor.
 * RU: Прочитать ресурс с одного заимствованного fid независимо от курсора скрипта. */
FVM_Result fvm_styxrpc_read_open_asset(StyxRpcVfsFile *, size_t, unsigned char **, size_t *);

/* EN: Read a nonempty file through one VFS fid up to limit bytes. Caller frees *bytes on success.
 * RU: Прочитать непустой файл до limit байт через один fid VFS. При успехе владелец освобождает *bytes. */
FVM_Result fvm_styxrpc_read_asset(struct StyxClient *session, const char *path, size_t limit,
    unsigned char **bytes, size_t *length);


/* [GC:] EN: Release or apply pending requests in order on the owner after a timeline call.
 * RU: Освободить либо применить запросы по порядку у владельца после выполнения кадра. */
void fvm_styxrpc_movies_finish(FVM_StyxRpcMovies *movies, int apply);

/* EN: Install file methods on attached rpc and expose os as the same object.
 * RU: Установить файловые методы в rpc и выставить тот же объект как os. */
FVM_Result fvm_styxrpc_os_install(FVM_StyxRpcMovies *);

#endif
