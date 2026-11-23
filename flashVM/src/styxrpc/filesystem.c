/* Copyright (c) 2026 bhgv
 * EN: Adapt ordinary FlashVM files to the mounted libstyxrpc VFS.
 * RU: Адаптация обычных файлов FlashVM к смонтированной VFS libstyxrpc.
 */
#include "flashvm/styxrpc_filesystem.h"
#include "styxrpc/vfs_client.h"
#include <string.h>

#define LIMB_BITS 32U /* Width of a script stat length limb. */

typedef struct DirectoryPage
{
    FVM_FsVisit visit; /* Owner's visitor borrowing one live entry. */
    void *state;      /* Borrowed visitor context. */
    FVM_Result status; /* First allocation or publication error from the owner. */
} DirectoryPage;


/* EN: Map admission, confirmed bytes, and transport failures to ordinary file status.
 * RU: Преобразовать допуск, подтверждённые байты и сбои в обычный статус файла. */
static FVM_Result result(StyxRpcIoOutcome outcome)
{
    return outcome == STYXRPC_IO_OK ? FVM_OK : outcome == STYXRPC_IO_NOT_OPEN ? FVM_NOT_FOUND : FVM_ERROR;
}


/* EN: Open a regular file on the exact mount. Supported modes work on existing files.
 * RU: Открыть обычный файл точного монтирования. Поддерживаемые режимы работают с существующим файлом. */
static FVM_Result open_file(void *session, const char *path, const char *mode, void **handle)
{
    StyxRpcFileMode access;
    StyxRpcVfsFile *file;
    FVM_Result status;
    if (!strcmp(mode, "r"))
    {
        access = STYXRPC_FILE_READ;
    }
    else if (!strcmp(mode, "wo"))
    {
        access = STYXRPC_FILE_WRITE;
    }
    else if (!strcmp(mode, "rwo") || !strcmp(mode, "rw"))
    {
        access = STYXRPC_FILE_RDWR;
    }
    else
    {
        return FVM_UNSUPPORTED;
    }
    status = result(styxrpc_vfs_open(session, path, access, &file));
    if (status == FVM_OK)
    {
        *handle = file;
    }
    return status;
}


/* EN: Borrow the exact open fid and read across SDK-sized chunks until EOF or failure.
 * RU: Читать с точного открытого fid фрагментами SDK до EOF либо ошибки. */
static FVM_Result read_file(void *file, long offset, void *bytes, size_t size, size_t *used)
{
    FVM_Result status = FVM_OK;
    *used = 0;
    while (*used < size)
    {
        size_t count;
        size_t chunk = size - *used;
        if (chunk > STYXRPC_VFS_TRANSFER_MAX)
        {
            chunk = STYXRPC_VFS_TRANSFER_MAX;
        }
        status = result(styxrpc_vfs_file_read(file, (uint64_t)offset + *used, (unsigned char *)bytes + *used, chunk, &count));
        *used += count;
        if (status != FVM_OK || count != chunk)
        {
            break;
        }
    }
    return status;
}


/* EN: Write consecutive confirmed chunks on the held fid; stop at the first incomplete chunk.
 * RU: Записать последовательные подтверждённые фрагменты через удерживаемый fid до первого неполного. */
static FVM_Result write_file(void *file, long offset, const void *bytes, size_t size)
{
    size_t used;
    for (used = 0; used < size;)
    {
        size_t count;
        size_t chunk = size - used;
        if (chunk > STYXRPC_VFS_TRANSFER_MAX)
        {
            chunk = STYXRPC_VFS_TRANSFER_MAX;
        }
        if (styxrpc_vfs_file_write(file, (uint64_t)offset + used, (const unsigned char *)bytes + used,
                chunk, &count) != STYXRPC_IO_OK || count != chunk)
        {
            return FVM_ERROR;
        }
        used += count;
    }
    return FVM_OK;
}


/* EN: Get length on the open fid for seek or append; preserve size on errors.
 * RU: Получить длину открытого fid для seek либо дописывания. При ошибке сохранить size. */
static FVM_Result file_size(void *file, size_t *size)
{
    StyxRpcFileInfo info;
    FVM_Result status = result(styxrpc_vfs_file_stat(file, &info));
    if (status == FVM_OK)
    {
        size_t extent = (size_t)info.length;
        if ((uint64_t)extent != info.length)
        {
            return FVM_UNSUPPORTED;
        }
        *size = extent;
    }
    return status;
}


/* [GC:] EN: Consume one open fid with deferred transport close.
 * RU: Освободить один открытый fid с отложенным закрытием транспорта. */
static void close_file(void *file)
{
    styxrpc_vfs_close(file, 1);
}


/* EN: Convert a native metadata record to common file fields.
 * RU: Преобразовать нативную запись метаданных к общим полям файла. */
static void copy_info(FVM_FsInfo *out, const StyxRpcFileInfo *info)
{
    out->length_high = (unsigned long)(info->length >> LIMB_BITS);
    out->length_low = (unsigned long)(uint32_t)info->length;
    out->mode = info->mode;
    out->directory = info->directory;
}


/* EN: Read ordinary path metadata from the exact attached tree.
 * RU: Прочитать обычные метаданные пути из точного смонтированного дерева. */
static FVM_Result stat_path(void *session, const char *path, FVM_FsInfo *out)
{
    StyxRpcFileInfo info;
    FVM_Result status = result(styxrpc_vfs_stat(session, path, &info));
    if (status == FVM_OK)
    {
        copy_info(out, &info);
    }
    return status;
}


/* EN: Convert and forward one borrowed directory entry. Stop on a visitor error.
 * RU: Преобразовать и передать заимствованную запись каталога, остановиться при ошибке. */
static int visit_entry(void *state, const char *name, const StyxRpcFileInfo *info)
{
    DirectoryPage *page = state;
    FVM_FsInfo entry;
    copy_info(&entry, info);
    page->status = page->visit(page->state, name, &entry);
    return page->status == FVM_OK;
}


/* EN: Visit one page and preserve the visitor's exact error if it stops enumeration.
 * RU: Посетить одну страницу и сохранить точную ошибку остановившего обход колбэка. */
static FVM_Result list_path(void *session, const char *path, size_t skip, size_t limit, FVM_FsVisit visit, void *state)
{
    DirectoryPage page;
    FVM_Result status;
    page.visit = visit;
    page.state = state;
    page.status = FVM_OK;
    status = result(styxrpc_vfs_list(session, path, skip, limit, visit_entry, &page));
    return page.status != FVM_OK ? page.status : status;
}


/* EN/RU: See flashvm/styxrpc_filesystem.h. / См. flashvm/styxrpc_filesystem.h. */
FVM_Result fvm_styxrpc_filesystem_mount(FVM_Context *context, const char *path, struct StyxClient *session)
{
    static const FVM_Filesystem backend = {{read_file, write_file, file_size, close_file}, open_file, stat_path, list_path};
    return session ? fvm_filesystem_mount(context, path, &backend, session) : FVM_INVALID_ARGUMENT;
}
