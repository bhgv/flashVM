/* Copyright (c) 2026 bhgv
 * EN: Wait for named rendezvous over the current Styx attachment.
 * RU: Дождаться именованного обмена на текущем подключении Styx.
 */
#define _POSIX_C_SOURCE 200809L /* nanosleep on hosted Unix builds. */
#include "styxrpc/named_cell.h"
#include "client.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define STYXRPC_NAMED_POLL_NS 10000000L /* Idle polling interval, ten milliseconds. */


/* EN: Write a header then its body, accepting only wholly confirmed transfers.
 * RU: Записать заголовок и тело с подтверждением всех переданных байтов. */
static StyxRpcIoOutcome submit(StyxRpcVfsFile *file, unsigned operation, const void *bytes, size_t size)
{
    unsigned char header[STYXRPC_NAMED_HEADER] = {(unsigned char)operation, (unsigned char)(size >> 24),
        (unsigned char)(size >> 16), (unsigned char)(size >> 8), (unsigned char)size};
    size_t used;
    StyxRpcIoOutcome status = styxrpc_vfs_file_write(file, 0, header, sizeof(header), &used);
    if (status != STYXRPC_IO_OK || used != sizeof(header))
    {
        return status == STYXRPC_IO_OK ? STYXRPC_IO_FAILED : status;
    }
    if (!size)
    {
        return STYXRPC_IO_OK;
    }
    status = styxrpc_vfs_file_write(file, sizeof(header), bytes, size, &used);
    return status == STYXRPC_IO_OK && used != size ? STYXRPC_IO_FAILED : status;
}


/* EN: Open a blank control fid for a named endpoint or a separate cell lease.
 * RU: Открыть пустой управляющий fid для канала либо отдельной аренды ячейки. */
static StyxRpcVfsFile *open_service(struct StyxClient *session)
{
    StyxRpcVfsFile *file = session ? styxclientwalk(session, STYXRPC_NAMED_SERVICE) : NULL;
    if (file && styxclientopen(file, STYXRPC_FILE_RDWR))
    {
        return file;
    }
    styxrpc_vfs_close(file, 0);
    return NULL;
}


/* EN/RU: See named_channel.h. / См. named_channel.h. */
StyxRpcVfsFile *styxrpc_named_open(struct StyxClient *session, const char *path)
{
    if (!session || !styxrpc_named_path(path))
    {
        return NULL;
    }
    StyxRpcVfsFile *file = open_service(session);
    if (file && submit(file, STYXRPC_NAMED_OPEN, path, strlen(path)) == STYXRPC_IO_OK)
    {
        return file;
    }
    styxrpc_vfs_close(file, 0);
    return NULL;
}


/* EN: Optional stop hook on one polling rendezvous. A NULL wait means one probe.
 * RU: Колбэк отмены одного ожидания пары. Пустой wait означает один опрос. */
typedef struct NamedWait
{
    StyxRpcNamedStop stop; /* Borrowed until this synchronous operation returns. */
    void *state;         /* Borrowed stop-hook data. */
} NamedWait;


/* EN: Ask the optional stop hook when no transmitted request owns the caller.
 * RU: Проверить отмену между протокольными запросами. */
static int cancelled(const NamedWait *wait)
{
    return wait && wait->stop && wait->stop(wait->state);
}


/* EN: Read the reply length. A non-NULL wait permits pending replies and cancellation.
 * RU: Прочитать длину ответа. Непустой wait допускает ожидание и отмену. */
static StyxRpcIoOutcome wait_reply(StyxRpcVfsFile *file, size_t *size, const NamedWait *wait)
{
    unsigned char header[STYXRPC_NAMED_REPLY];
    const struct timespec pause = {0, STYXRPC_NAMED_POLL_NS};
    for (;;)
    {
        size_t used;
        if (cancelled(wait))
        {
            return STYXRPC_IO_CANCELLED;
        }
        StyxRpcIoOutcome status = styxrpc_vfs_file_read(file, 0, header, sizeof(header), &used);
        if (status != STYXRPC_IO_OK || (used && used != sizeof(header)))
        {
            return status == STYXRPC_IO_OK ? STYXRPC_IO_FAILED : status;
        }
        if (used)
        {
            *size = ((uint32_t)header[0] << 24) | ((uint32_t)header[1] << 16) |
                ((uint32_t)header[2] << 8) | header[3];
            return STYXRPC_IO_OK;
        }
        if (!wait)
        {
            return STYXRPC_IO_FAILED;
        }
        nanosleep(&pause, NULL);
    }
}


/* EN: Read one nonempty, bounded reply body on file. With wait set, await a channel peer.
 * On success, give ownership of the buffer in bytes and its length in size to the caller.
 * RU: Прочитать непустое ограниченное тело ответа file. При wait дождаться участника канала.
 * При успехе передать буфер bytes во владение и вернуть его размер в size. */
static StyxRpcIoOutcome read_frame(StyxRpcVfsFile *file, const NamedWait *wait, unsigned char **bytes, size_t *size)
{
    size_t length;
    StyxRpcIoOutcome status = wait_reply(file, &length, wait);
    if (status != STYXRPC_IO_OK || !length || length > STYXRPC_NAMED_MAX)
    {
        return status == STYXRPC_IO_OK ? STYXRPC_IO_FAILED : status;
    }
    unsigned char *frame = malloc(length);
    if (!frame)
    {
        return STYXRPC_IO_FAILED;
    }
    size_t used;
    status = styxrpc_vfs_file_read(file, STYXRPC_NAMED_REPLY, frame, length, &used);
    if (status != STYXRPC_IO_OK || used != length)
    {
        free(frame);
        return status == STYXRPC_IO_OK ? STYXRPC_IO_FAILED : status;
    }
    *bytes = frame;
    *size = length;
    return STYXRPC_IO_OK;
}


/* EN: Submit a zero-body receive or get request and fetch its reply.
 * RU: Отправить пустой запрос приёма или чтения ячейки и получить ответ. */
static StyxRpcIoOutcome receive(StyxRpcVfsFile *file, unsigned operation, unsigned char **bytes, size_t *size,
    const NamedWait *wait)
{
    if (!file || !bytes || !size)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    if (cancelled(wait))
    {
        return STYXRPC_IO_CANCELLED;
    }
    StyxRpcIoOutcome status = submit(file, operation, NULL, 0);
    return status == STYXRPC_IO_OK ? read_frame(file, wait, bytes, size) : status;
}


/* EN/RU: See named_channel.h. / См. named_channel.h. */
StyxRpcIoOutcome styxrpc_named_recv(StyxRpcVfsFile *file, unsigned char **bytes, size_t *size)
{
    return styxrpc_named_recv_until(file, bytes, size, NULL, NULL);
}


/* EN/RU: See named_channel.h. / См. named_channel.h. */
StyxRpcIoOutcome styxrpc_named_send(StyxRpcVfsFile *file, const void *bytes, size_t size)
{
    return styxrpc_named_send_until(file, bytes, size, NULL, NULL);
}


/* EN/RU: See named_channel.h. / См. named_channel.h. */
StyxRpcIoOutcome styxrpc_named_recv_until(StyxRpcVfsFile *file, unsigned char **bytes, size_t *size,
    StyxRpcNamedStop stop, void *state)
{
    const NamedWait wait = {stop, state};
    return receive(file, STYXRPC_NAMED_READ, bytes, size, &wait);
}


/* EN/RU: See named_channel.h. / См. named_channel.h. */
StyxRpcIoOutcome styxrpc_named_send_until(StyxRpcVfsFile *file, const void *bytes, size_t size,
    StyxRpcNamedStop stop, void *state)
{
    const NamedWait wait = {stop, state};
    if (!file || !bytes || !size || size > STYXRPC_NAMED_MAX)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    if (cancelled(&wait))
    {
        return STYXRPC_IO_CANCELLED;
    }
    StyxRpcIoOutcome status = submit(file, STYXRPC_NAMED_WRITE, bytes, size);
    size_t length;
    if (status == STYXRPC_IO_OK)
    {
        status = wait_reply(file, &length, &wait);
    }
    return status == STYXRPC_IO_OK && length ? STYXRPC_IO_FAILED : status;
}


/* EN/RU: See named_cell.h. / См. named_cell.h. */
StyxRpcVfsFile *styxrpc_cell_create(struct StyxClient *session, const void *frame, size_t size, void *token)
{
    if (!session || !token || !styxrpc_cell_value(frame, size))
    {
        return NULL;
    }
    StyxRpcVfsFile *file = open_service(session);
    StyxRpcIoOutcome status = file ? submit(file, STYXRPC_CELL_NEW, frame, size) : STYXRPC_IO_NOT_OPEN;
    unsigned char *reply = NULL;
    size_t returned;
    if (status == STYXRPC_IO_OK && read_frame(file, NULL, &reply, &returned) == STYXRPC_IO_OK &&
        returned == STYXRPC_CELL_TOKEN)
    {
        memcpy(token, reply, STYXRPC_CELL_TOKEN);
        free(reply);
        return file;
    }
    free(reply);
    styxrpc_vfs_close(file, 0);
    return NULL;
}


/* EN/RU: See named_cell.h. / См. named_cell.h. */
StyxRpcVfsFile *styxrpc_cell_attach(struct StyxClient *session, const void *token)
{
    if (!session || !token)
    {
        return NULL;
    }
    StyxRpcVfsFile *file = open_service(session);
    if (file && submit(file, STYXRPC_CELL_ATTACH, token, STYXRPC_CELL_TOKEN) == STYXRPC_IO_OK)
    {
        return file;
    }
    styxrpc_vfs_close(file, 0);
    return NULL;
}


/* EN/RU: See named_cell.h. / См. named_cell.h. */
StyxRpcIoOutcome styxrpc_cell_get(StyxRpcVfsFile *file, unsigned char **bytes, size_t *size)
{
    if (!bytes || !size)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    unsigned char *frame;
    size_t length;
    StyxRpcIoOutcome status = receive(file, STYXRPC_CELL_GET, &frame, &length, NULL);
    if (status != STYXRPC_IO_OK)
    {
        return status;
    }
    if (!styxrpc_cell_value(frame, length))
    {
        free(frame);
        return STYXRPC_IO_FAILED;
    }
    *bytes = frame;
    *size = length;
    return STYXRPC_IO_OK;
}


/* EN/RU: See named_cell.h. / См. named_cell.h. */
StyxRpcIoOutcome styxrpc_cell_set(StyxRpcVfsFile *file, const void *bytes, size_t size)
{
    if (!file || !styxrpc_cell_value(bytes, size))
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    return submit(file, STYXRPC_CELL_SET, bytes, size);
}
