/* Copyright (c) 2026 bhgv
 * EN: Independent file cursors, including opens relative to owned directory anchors.
 * RU: Независимые file cursors, включая open от directory anchors.
 */
/* FlashVM: Portable platform adapter interface or implementation: file port.
 * File: ports/file_port.c.
 */

#include "file_port.h"
#include "filesystem_native.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>

#else
#include <unistd.h>
#endif


#define FVM_STDIO_TRANSFER_MAX 65536UL /* Maximum native standard-stream fragment accepted per call. */


/* Owned file state keeps logical read/write positions independent. */
struct FVM_PortFile
{
    FILE *stream;        /* Owned binary stream; standard descriptors use separate borrowed APIs. */
    char *path;          /* Owned UTF-8 display path; metadata never reopens it. */
    long read_position;  /* Logical next read offset, independent of appending writes. */
    long write_position; /* Logical next write offset checked against native long capacity. */
    int readable;        /* Whether this handle admits ordinary file reads. */
    int writable;        /* Whether this handle admits ordinary file writes. */
    int shared;          /* rwo uses one logical offset for both reading and writing. */
    int append;          /* Whether each write is positioned at the current file end. */
    int failed;          /* A partial/failed record prevents unsafe write retries. */
};


/* Open borrowed UTF-8 path in r, w, wx, wo, rwo or rw mode (NULL means rw).
 * out receives an owned file or NULL; close it with fvm_port_file_close.
 * Validate path/mode and allocate state before destructive creation.
 * Native I/O blocks; invalid/unsupported paths, allocation and OS failures
 * return explicit status. External synchronization is required.
 * RU: Открыть path относительно anchor либо native path; передать свой file
 * в out или вернуть ошибку с NULL. wo не создаёт и не усекает файл, позиция ноль.
 * До destructive effects выделить state; блокирует, требует внешней синхронизации. */
static FVM_Result file_open(FVM_PortAnchor *anchor, const char *path, const char *mode, FVM_PortFile **out)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_PortFile *file;
    FILE *stream;
    int read, write;
    FVM_Result path_status;
    if (!out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *out = 0;
    if (!path || !*path || (anchor && *path == '/'))
    {
        return FVM_INVALID_ARGUMENT;
    }
    path_status = fvm_fs_native_path(path);
    if (path_status != FVM_OK)
    {
        return path_status;
    }
    if (!mode)
    {
        mode = "rw";
    }

    /* Computation / Вычисление и обновление состояния. */
    read = !strcmp(mode, "r") || !strcmp(mode, "rw") || !strcmp(mode, "rwo");
    write = !strcmp(mode, "w") || !strcmp(mode, "wx") || !strcmp(mode, "wo") || !strcmp(mode, "rw") || !strcmp(mode, "rwo");
    if (!read && !write)
    {
        return FVM_INVALID_ARGUMENT;
    }
    /* Allocate before a destructive w open: allocation failure must not
       truncate an existing file. Only rw/ENOENT may create a missing file. */
    file = (FVM_PortFile *)calloc(1, sizeof(*file));
    if (!file)
    {
        return FVM_OUT_OF_MEMORY;
    }
    file->path = (char *)malloc(strlen(path) + 1);
    if (!file->path)
    {
        free(file);
        return FVM_OUT_OF_MEMORY;
    }
    strcpy(file->path, path);
    {
        const char *binary = !strcmp(mode, "wo") ? "wb-" : !strcmp(mode, "wx") ? "wbx" : (read ? (write ? "r+b" : "rb") : "wb");
        stream = (FILE *)(anchor ? fvm_fs_native_open_at(anchor, path, binary) : fvm_fs_native_open(path, binary));
    }
    if (!stream && !strcmp(mode, "rw") && errno == ENOENT)
    {
        stream = (FILE *)(anchor ? fvm_fs_native_open_at(anchor, path, "w+b") : fvm_fs_native_open(path, "w+b"));
    }
    if (!stream)
    {
        free(file->path);
        free(file);
        return FVM_ERROR;
    }

    /* Computation / Вычисление и обновление состояния. */
    file->stream = stream;
    file->readable = read;
    file->writable = write;
    file->shared = !strcmp(mode, "rwo");
    file->append = read && write && !file->shared;
    if (write && strcmp(mode, "wo") && !file->shared && (fseek(stream, 0, SEEK_END) || (file->write_position = ftell(stream)) < 0))
    {
        fclose(stream);
        free(file->path);
        free(file);
        return FVM_ERROR;
    }
    *out = file;
    return FVM_OK;
}


/* EN/RU: Open borrowed native path; see file_port.h for ownership and errors.
 * Открыть borrowed native path; владение/ошибки описаны в file_port.h. */
FVM_Result fvm_port_file_open(const char *path, const char *mode, FVM_PortFile **out)
{
    return file_open(NULL, path, mode, out);
}


/* EN/RU: See anchor_port.h. Parent stays owned through this blocking operation.
 * См. anchor_port.h. Parent удерживается до завершения blocking operation. */
FVM_Result fvm_port_file_open_at(FVM_PortAnchor *parent, const char *relative, const char *mode, FVM_PortFile **out)
{
    if (!parent)
    {
        if (out)
        {
            *out = NULL;
        }
        return FVM_INVALID_ARGUMENT;
    }
    return file_open(parent, relative, mode, out);
}


/* Close port file. */
void fvm_port_file_close(FVM_PortFile *file)
{
    if (!file)
    {
        return;
    }
    fclose(file->stream);
    free(file->path);
    free(file);
}


/* Read position of port file. */
long fvm_port_file_position(FVM_PortFile *file)
{
    return file->read_position;
}


/* Restore port file. */
void fvm_port_file_restore(FVM_PortFile *file, long position)
{
    file->read_position = position;
    if (file->shared)
    {
        file->write_position = position;
    }
}


/* EN/RU: Read at most size bytes; rwo advances one shared logical cursor.
 * Прочитать до size bytes; rwo продвигает общий logical cursor. Borrow inputs;
 * read reports confirmed bytes on success, no ownership transfer. */
FVM_Result fvm_port_file_read(FVM_PortFile *file, void *data, size_t size, size_t *read)
{
    size_t count;
    if (!file || !read || (!data && size))
    {
        return FVM_INVALID_ARGUMENT;
    }
    *read = 0;
    if (!file->readable || file->failed)
    {
        return FVM_ERROR;
    }
    if (size > (size_t)(LONG_MAX - file->read_position))
    {
        return FVM_UNSUPPORTED;
    }
    clearerr(file->stream);
    if (fseek(file->stream, file->read_position, SEEK_SET))
    {
        return FVM_ERROR;
    }
    count = fread(data, 1, size, file->stream);
    if (ferror(file->stream))
    {
        return FVM_ERROR;
    }

    /* Computation / Вычисление и обновление состояния. */
    file->read_position += (long)count;
    if (file->shared)
    {
        file->write_position = file->read_position;
    }
    *read = count;
    return FVM_OK;
}


/* EN/RU: Write serialized bytes; rwo overwrites at the shared cursor.
 * Записать serialized bytes; rwo пишет у общего cursor. No create/append in rwo;
 * failed legacy writes keep their existing uncertain-effect failure contract. */
FVM_Result fvm_port_file_write(FVM_PortFile *file, const void *data, size_t size)
{
    if (!file || (!data && size))
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!file->writable || file->failed)
    {
        return FVM_ERROR;
    }
    if (file->append && (fseek(file->stream, 0, SEEK_END) || (file->write_position = ftell(file->stream)) < 0))
    {
        return FVM_ERROR;
    }
    if (size > (size_t)(LONG_MAX - file->write_position))
    {
        return FVM_UNSUPPORTED;
    }
    clearerr(file->stream);
    if (fseek(file->stream, file->write_position, SEEK_SET) || fwrite(data, 1, size, file->stream) != size ||
        fflush(file->stream))
    { /* A partial record cannot be retried on this handle. */
        file->failed = 1;
        return FVM_ERROR;
    }
    file->write_position += (long)size;
    if (file->shared)
    {
        file->read_position = file->write_position;
    }
    return FVM_OK;
}


/* Compute and return port file available. */
FVM_Result fvm_port_file_available(FVM_PortFile *file, int *available)
{

    /* Initialization / Инициализация локального состояния. */
    unsigned char byte;
    size_t count;
    long position;
    FVM_Result result;

    /* Computation / Вычисление и обновление состояния. */
    position = file->read_position;
    result = fvm_port_file_read(file, &byte, 1, &count);
    fvm_port_file_restore(file, position);
    *available = result == FVM_OK && count != 0;
    return result;
}


/* Measure the already-open binary file without changing logical read/write positions.
 * file is borrowed; size changes only on success. Returns an explicit port error. */
FVM_Result fvm_port_file_size(FVM_PortFile *file, size_t *size)
{
    long saved;
    long end;
    if (!file || !size)
    {
        return FVM_INVALID_ARGUMENT;
    }
    saved = ftell(file->stream);
    if (saved < 0 || fseek(file->stream, 0, SEEK_END) != 0)
    {
        return FVM_ERROR;
    }
    end = ftell(file->stream);
    if (fseek(file->stream, saved, SEEK_SET) != 0 || end < 0)
    {
        return FVM_ERROR;
    }
    *size = (size_t)end;
    return FVM_OK;
}


/* Move the single active cursor without changing append-channel semantics.
 * file is borrowed; offset/origin choose a checked nonnegative native-long offset.
 * position changes only on success. Caller serializes access; native seek may block. */
FVM_Result fvm_port_file_seek(FVM_PortFile *file, long offset, int origin, long *position)
{
    long base;
    long target;
    size_t extent;
    if (!file || !position || origin < FVM_FILE_SEEK_START || origin > FVM_FILE_SEEK_END)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (file->append)
    {
        return FVM_UNSUPPORTED;
    }
    if (file->failed)
    {
        return FVM_ERROR;
    }
    base = 0;
    if (origin == FVM_FILE_SEEK_CURRENT)
    {
        base = file->readable ? file->read_position : file->write_position;
    }
    else if (origin == FVM_FILE_SEEK_END)
    {
        if (fvm_port_file_size(file, &extent) != FVM_OK || extent > (size_t)LONG_MAX)
        {
            return FVM_ERROR;
        }
        base = (long)extent;
    }
    if (base < 0)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if ((offset < 0 && offset < -base) || (offset > 0 && offset > LONG_MAX - base))
    {
        return FVM_INVALID_ARGUMENT;
    }
    target = base + offset;
    if (fseek(file->stream, target, SEEK_SET))
    {
        return FVM_ERROR;
    }
    if (file->readable)
    {
        file->read_position = target;
    }
    if (!file->readable || file->shared)
    {
        file->write_position = target;
    }
    *position = target;
    return FVM_OK;
}


/* Read one bounded fragment from borrowed standard input.
 * descriptor must be zero; output receives the actual extent, including EOF zero. */
FVM_Result fvm_port_stdio_read(int descriptor, void *bytes, size_t capacity, size_t *received)
{
    long count;
    if (descriptor != 0 || !received || (!bytes && capacity))
    {
        return FVM_INVALID_ARGUMENT;
    }
    *received = 0;
    if (!capacity)
    {
        return FVM_OK;
    }
    if (capacity > FVM_STDIO_TRANSFER_MAX)
    {
        capacity = FVM_STDIO_TRANSFER_MAX;
    }
    do
    {
#ifdef _WIN32
        count = _read(descriptor, bytes, (unsigned int)capacity);
#else
        count = (long)read(descriptor, bytes, capacity);
#endif
    } while (count < 0 && errno == EINTR);
    if (count < 0)
    {
        return FVM_ERROR;
    }
    *received = (size_t)count;
    return FVM_OK;
}


/* Write one bounded fragment to borrowed standard output/error.
 * descriptor is 1 or 2; written exposes partial writes so callers can retry the remainder. */
FVM_Result fvm_port_stdio_write(int descriptor, const void *bytes, size_t size, size_t *written)
{
    long count;
    if ((descriptor != 1 && descriptor != 2) || !written || (!bytes && size))
    {
        return FVM_INVALID_ARGUMENT;
    }
    *written = 0;
    if (!size)
    {
        return FVM_OK;
    }
    if (fflush(descriptor == 1 ? stdout : stderr))
    {
        return FVM_ERROR;
    }
    if (size > FVM_STDIO_TRANSFER_MAX)
    {
        size = FVM_STDIO_TRANSFER_MAX;
    }
    do
    {
#ifdef _WIN32
        count = _write(descriptor, bytes, (unsigned int)size);
#else
        count = (long)write(descriptor, bytes, size);
#endif
    } while (count < 0 && errno == EINTR);
    if (count < 0)
    {
        return FVM_ERROR;
    }
    *written = (size_t)count;
    return FVM_OK;
}


/* Query an actual open stream through the selected OS port. Borrow file/output;
 * metadata success does not change logical file positions or release ownership. */
FVM_Result fvm_port_file_info(FVM_PortFile *file, FVM_FileInfo *output)
{
    if (!file || !output)
    {
        return FVM_INVALID_ARGUMENT;
    }
    return fvm_fs_native_stream(file->stream, file->path, output);
}
