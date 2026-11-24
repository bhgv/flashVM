/* Copyright (c) 2026 bhgv
 * EN: Resolve movie paths and replace clips at the owner safe point through the held Styx session.
 * RU: Разрешение путей и замена клипов в безопасной точке владельца через сессию Styx.
 */
#include "movie_load.h"
#include "flashvm/gc.h"
#include "flashvm/movieclip.h"
#include <stdlib.h>
#include <string.h>


/* EN: Append one literal component or resolve dot navigation without crossing the attach root.
 * RU: Дописать буквальный компонент либо обработать точки в пределах корня сессии. */
static int add_component(char *path, size_t *used, const char *name, size_t size)
{
    if (!size || (size == sizeof(".prog") - 1 && !memcmp(name, ".prog", size)))
    {
        return 0;
    }
    if (size == 1 && name[0] == '.')
    {
        return 1;
    }
    if (size == 2 && name[0] == '.' && name[1] == '.')
    {
        if (*used == 1)
        {
            return 0;
        }
        do
        {
            --*used;
        } while (*used > 1 && path[*used] != '/');
        return 1;
    }
    if (size >= FVM_STYXRPC_MOVIE_PATH - *used - (*used > 1))
    {
        return 0;
    }
    if (*used > 1)
    {
        path[(*used)++] = '/';
    }
    memcpy(path + *used, name, size);
    *used += size;
    return 1;
}


/* EN: Build a canonical absolute VFS filename from counted data and an inherited file source.
 * RU: Собрать канонический абсолютный путь VFS из строки с длиной и источника клипа. */
static int resolve(const FVM_Value *base, const char *reference, size_t size, char *path)
{
    size_t used = 1;
    size_t first, i;
    if (!reference || !size || size >= FVM_STYXRPC_MOVIE_PATH || reference[size - 1] == '/')
    {
        return 0;
    }
    path[0] = '/';
    if (reference[0] != '/')
    {
        size_t directory = fvm_value_string_size(base);
        if (!directory || directory >= FVM_STYXRPC_MOVIE_PATH || base->string[0] != '/')
        {
            return 0;
        }
        while (base->string[directory - 1] != '/')
        {
            --directory;
        }
        used = directory > 1 ? directory - 1 : 1;
        memcpy(path, base->string, used);
    }
    first = reference[0] == '/' ? 1 : 0;
    for (i = first; i < size; ++i)
    {
        char byte = reference[i];
        if (!byte || byte == ':' || byte == '\\' || byte == '?' || byte == '#')
        {
            return 0;
        }
        if (byte == '/')
        {
            if (!add_component(path, &used, reference + first, i - first))
            {
                return 0;
            }
            first = i + 1;
        }
    }
    if (!add_component(path, &used, reference + first, size - first) || used == 1)
    {
        return 0;
    }
    path[used] = 0;
    return 1;
}


/* EN/RU: See movie_load.h. / См. movie_load.h. */
FVM_Result fvm_styxrpc_movie_reject(FVM_StyxRpcMovies *movies, FVM_Result result)
{
    ++movies->failed;
    movies->last = result;
    return FVM_OK;
}


/* [GC:] EN/RU: See movie_load.h. / См. movie_load.h. */
FVM_Result fvm_styxrpc_movie_queue(FVM_StyxRpcMovies *movies, FVM_Object *origin, FVM_Object *target,
    const char *reference, size_t length)
{
    FVM_Value source;
    FVM_StyxRpcMovieRequest *request;
    FVM_Result status;
    int attached = 0;
    if (movies->pending == FVM_STYXRPC_MOVIE_QUEUE)
    {
        return fvm_styxrpc_movie_reject(movies, FVM_UNSUPPORTED);
    }
    status = fvm_movieclip_attached_to(movies->context, target, movies->root, &attached);
    if (status != FVM_OK || !attached)
    {
        return fvm_styxrpc_movie_reject(movies, FVM_NOT_FOUND);
    }
    request = movies->queue + movies->pending;
    status = fvm_movieclip_get_source(movies->context, origin, &source);
    if (status != FVM_OK || !resolve(&source, reference, length, request->path))
    {
        return fvm_styxrpc_movie_reject(movies, status == FVM_OK ? FVM_INVALID_ARGUMENT : status);
    }
    status = fvm_gc_pin(movies->context, target);
    if (status != FVM_OK)
    {
        return status;
    }
    request->target = target;
    ++movies->pending;
    return FVM_OK;
}


/* EN: Read exactly the recorded extent on a held fid, checking EOF and confirmed progress.
 * RU: Прочитать точную длину с удерживаемого fid, проверив EOF и подтверждённый прогресс. */
static FVM_Result read_movie(StyxRpcVfsFile *file, unsigned char *bytes, size_t length)
{
    size_t offset = 0, received;
    unsigned char extra;
    while (offset < length)
    {
        size_t chunk = length - offset;
        if (chunk > STYXRPC_VFS_TRANSFER_MAX)
        {
            chunk = STYXRPC_VFS_TRANSFER_MAX;
        }
        if (styxrpc_vfs_file_read(file, offset, bytes + offset, chunk, &received) != STYXRPC_IO_OK ||
            received != chunk)
        {
            return FVM_ERROR;
        }
        offset += received;
    }
    return styxrpc_vfs_file_read(file, offset, &extra, sizeof(extra), &received) == STYXRPC_IO_OK && !received
        ? FVM_OK : FVM_ERROR;
}


/* EN: Stage one bounded SWF from the attached VFS and publish only after complete validation.
 * RU: Получить SWF ограниченного размера из VFS и опубликовать после полной проверки. */
static FVM_Result replace_movie(FVM_StyxRpcMovies *movies, const FVM_StyxRpcMovieRequest *request)
{
    StyxRpcVfsFile *file = NULL;
    StyxRpcFileInfo info;
    unsigned char *bytes;
    FVM_Result result;
    if (styxrpc_vfs_open(movies->session, request->path + 1, STYXRPC_FILE_READ, &file) != STYXRPC_IO_OK)
    {
        return FVM_NOT_FOUND;
    }
    if (styxrpc_vfs_file_stat(file, &info) != STYXRPC_IO_OK)
    {
        styxrpc_vfs_close(file, 0);
        return FVM_ERROR;
    }
    if (!info.length || info.length > movies->limit)
    {
        styxrpc_vfs_close(file, 0);
        return info.length ? FVM_UNSUPPORTED : FVM_INVALID_ARGUMENT;
    }
    bytes = malloc((size_t)info.length);
    result = bytes ? read_movie(file, bytes, (size_t)info.length) : FVM_OUT_OF_MEMORY;
    styxrpc_vfs_close(file, 0);
    if (result == FVM_OK)
    {
        result = fvm_movieclip_replace_swf(movies->context, request->target, bytes, (size_t)info.length,
            request->path, strlen(request->path));
    }
    free(bytes);
    return result;
}


/* [GC:] EN/RU: See movie_load.h. / См. movie_load.h. */
void fvm_styxrpc_movies_finish(FVM_StyxRpcMovies *movies, int apply)
{
    size_t index;
    if (!movies)
    {
        return;
    }
    for (index = 0; index < movies->pending; ++index)
    {
        FVM_StyxRpcMovieRequest *request = movies->queue + index;
        if (apply)
        {
            int attached = 0;
            FVM_Result status = fvm_movieclip_attached_to(movies->context, request->target, movies->root, &attached);
            if (status == FVM_OK)
            {
                status = attached ? replace_movie(movies, request) : FVM_NOT_FOUND;
            }
            if (status == FVM_OK)
            {
                ++movies->loaded;
                movies->last = FVM_OK;
            }
            else
            {
                fvm_styxrpc_movie_reject(movies, status);
            }
        }
        fvm_gc_unpin(movies->context, request->target);
    }
    movies->pending = 0;
}
