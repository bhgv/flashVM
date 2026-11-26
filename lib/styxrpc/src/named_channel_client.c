/* Copyright (c) 2026 bhgv
 * EN: Wait for named rendezvous over the current Styx attachment.
 * RU: Дождаться именованного обмена на текущем подключении Styx.
 */
#define _POSIX_C_SOURCE 200809L /* nanosleep on hosted Unix builds. */
#include "styxrpc/named_channel.h"
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


/* EN/RU: See named_channel.h. / См. named_channel.h. */
StyxRpcVfsFile *styxrpc_named_open(struct StyxClient *session, const char *path)
{
    if (!session || !styxrpc_named_path(path))
    {
        return NULL;
    }
    StyxRpcVfsFile *file = styxclientwalk(session, STYXRPC_NAMED_SERVICE);
    if (file && styxclientopen(file, STYXRPC_FILE_RDWR) &&
        submit(file, STYXRPC_NAMED_OPEN, path, strlen(path)) == STYXRPC_IO_OK)
    {
        return file;
    }
    styxrpc_vfs_close(file, 0);
    return NULL;
}


/* EN: Wait without holding the Styx request lock. Copy one exact reply length to size.
 * RU: Ожидать без удержания блокировки Styx. Выдать точную длину ответа в size. */
static StyxRpcIoOutcome wait_reply(StyxRpcVfsFile *file, size_t *size)
{
    unsigned char header[STYXRPC_NAMED_REPLY];
    const struct timespec pause = {0, STYXRPC_NAMED_POLL_NS};
    for (;;)
    {
        size_t used;
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
        nanosleep(&pause, NULL);
    }
}


/* EN/RU: See named_channel.h. / См. named_channel.h. */
StyxRpcIoOutcome styxrpc_named_read(StyxRpcVfsFile *file, unsigned char **bytes, size_t *size)
{
    if (!file || !bytes || !size)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxRpcIoOutcome status = submit(file, STYXRPC_NAMED_READ, NULL, 0);
    size_t length;
    if (status == STYXRPC_IO_OK)
    {
        status = wait_reply(file, &length);
    }
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


/* EN/RU: See named_channel.h. / См. named_channel.h. */
StyxRpcIoOutcome styxrpc_named_write(StyxRpcVfsFile *file, const void *bytes, size_t size)
{
    if (!file || !bytes || !size || size > STYXRPC_NAMED_MAX)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxRpcIoOutcome status = submit(file, STYXRPC_NAMED_WRITE, bytes, size);
    size_t length;
    if (status == STYXRPC_IO_OK)
    {
        status = wait_reply(file, &length);
    }
    return status == STYXRPC_IO_OK && length ? STYXRPC_IO_FAILED : status;
}
