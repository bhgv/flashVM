/* FlashVM: Portable platform adapter interface or implementation: file port.
 * File: ports/file_port.c.
 */

#include "file_port.h"
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
    long read_position;  /* Logical next read offset, independent of appending writes. */
    long write_position; /* Logical next write offset checked against native long capacity. */
    int readable;        /* Whether this handle admits ordinary file reads. */
    int writable;        /* Whether this handle admits ordinary file writes. */
    int append;          /* Whether each write is positioned at the current file end. */
    int failed;          /* A partial/failed record prevents unsafe write retries. */
};


/* Open path. */
static FILE *open_path(const char *path, const char *mode)
{
#ifdef _WIN32
    wchar_t *wide;
    wchar_t wide_mode[4];
    FILE *stream;
    int count, i;
    count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, 0, 0);
    if (!count)
    {
        errno = EINVAL;
        return 0;
    }
    wide = (wchar_t *)malloc((size_t)count * sizeof(*wide));
    if (!wide)
    {
        errno = ENOMEM;
        return 0;
    }
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wide, count))
    {
        free(wide);
        errno = EINVAL;
        return 0;
    }
    for (i = 0; mode[i] && i < 3; ++i)
    {
        wide_mode[i] = (wchar_t)mode[i];
    }
    wide_mode[i] = 0;
    stream = _wfopen(wide, wide_mode);
    free(wide);
    return stream;
#else
    return fopen(path, mode);
#endif
}


/* Open port file. */
FVM_Result fvm_port_file_open(const char *path, const char *mode, FVM_PortFile **out)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_PortFile *file;
    FILE *stream;
    int read, write;
    if (!out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *out = 0;
    if (!path || !*path)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!mode)
    {
        mode = "rw";
    }

    /* Computation / Вычисление и обновление состояния. */
    read = !strcmp(mode, "r") || !strcmp(mode, "rw");
    write = !strcmp(mode, "w") || !strcmp(mode, "rw");
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
    stream = open_path(path, read ? (write ? "r+b" : "rb") : "wb");
    if (!stream && read && write && errno == ENOENT)
    {
        stream = open_path(path, "w+b");
    }
    if (!stream)
    {
        free(file);
        return FVM_ERROR;
    }

    /* Computation / Вычисление и обновление состояния. */
    file->stream = stream;
    file->readable = read;
    file->writable = write;
    file->append = read && write;
    if (write && (fseek(stream, 0, SEEK_END) || (file->write_position = ftell(stream)) < 0))
    {
        fclose(stream);
        free(file);
        return FVM_ERROR;
    }
    *out = file;
    return FVM_OK;
}


/* Close port file. */
void fvm_port_file_close(FVM_PortFile *file)
{
    if (!file)
    {
        return;
    }
    fclose(file->stream);
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
}


/* Read port file. */
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
    *read = count;
    return FVM_OK;
}


/* Write port file. */
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
    file->read_position = position;
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
    else
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
