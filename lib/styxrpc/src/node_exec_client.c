/* Copyright (c) 2026 bhgv
 * EN: Submit one executable node request on a private Styx fid.
 * RU: Выполнить один запрос к узлу через отдельный fid Styx.
 */
#include "styxrpc/nodes.h"
#include "client.h"
#include "node_exec_wire.h"
#include "vfs_client_internal.h"
#include <stdlib.h>
#include <string.h>


/* EN: Encode a checked count into one big-endian header word.
 * RU: Записать проверенный счётчик в слово заголовка старшим байтом вперёд. */
static void put_count(unsigned char *word, size_t value)
{
    for (size_t i = NODE_EXEC_WORD; i > 0; --i)
    {
        word[i - 1] = (unsigned char)value;
        value >>= 8;
    }
}


/* EN: Write the full span on the same fid. Short success cannot finish a request.
 * RU: Записать всё на тот же fid. Короткое подтверждение не завершает запрос. */
static StyxRpcIoOutcome write_span(StyxRpcVfsFile *file, size_t offset, const void *bytes, size_t size)
{
    size_t sent;
    StyxRpcIoOutcome outcome = styxrpc_vfs_file_write(file, offset, bytes, size, &sent);
    return outcome == STYXRPC_IO_OK && sent != size ? STYXRPC_IO_FAILED : outcome;
}


/* EN: Require the slot service's exact metadata before opening a walked fid.
 * RU: Перед открытием пройденного fid требовать точные метаданные службы слота. */
static int open_service(StyxFid *file)
{
    StyxMeta info;
    return file && styxclientstat(file, &info) && !info.directory &&
        info.identity == STYXRPC_NODE_EXEC_QID && !strcmp(info.name, STYXRPC_NODE_EXEC_SERVICE) &&
        styxclientopen(file, STYXRPC_FILE_RDWR);
}


/* EN/RU: See nodes.h. / См. nodes.h. */
StyxRpcIoOutcome styxrpc_node_exec(StyxClient *session, const char *path,
    void *bytes, size_t size, size_t *used)
{
    if (!used)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    *used = 0;
    if (!session || !styxrpc_vfs_visible_path(path) || (!bytes && size) || size > STYXRPC_NODE_BUFFER_MAX)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    unsigned char *reply = size ? malloc(size) : NULL;
    if (size && !reply)
    {
        return STYXRPC_IO_FAILED;
    }
    StyxFid *file = styxclientwalk(session, STYXRPC_NODE_EXEC_SERVICE);
    StyxRpcIoOutcome outcome = STYXRPC_IO_NOT_OPEN;
    if (open_service(file))
    {
        unsigned char header[NODE_EXEC_HEADER];
        size_t path_size = strlen(path) + 1;
        put_count(header, path_size);
        put_count(header + NODE_EXEC_WORD, size);
        outcome = write_span(file, 0, header, sizeof(header));
        if (outcome == STYXRPC_IO_OK)
        {
            outcome = write_span(file, sizeof(header), path, path_size);
        }
        if (outcome == STYXRPC_IO_OK && size)
        {
            outcome = write_span(file, sizeof(header) + path_size, bytes, size);
        }
        if (outcome == STYXRPC_IO_OK && size)
        {
            size_t received;
            outcome = styxrpc_vfs_file_read(file, 0, reply, size, &received);
            if (outcome == STYXRPC_IO_OK)
            {
                memcpy(bytes, reply, received);
                *used = received;
            }
        }
    }
    styxclientclunk(file);
    free(reply);
    return outcome;
}
