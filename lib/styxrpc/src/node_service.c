/* Copyright (c) 2026 bhgv
 * EN: Assemble one invocation per hidden service fid and expose its immutable result.
 * RU: Собрать один вызов на скрытый fid и выдать его неизменяемый результат.
 */
#include "styxrpc/node_service.h"
#include "node_exec_wire.h"
#include <stdlib.h>
#include <string.h>

typedef enum ExecPhase
{
    EXEC_REQUEST, /* Accept sequential request fragments. */
    EXEC_RESULT,  /* Execute completed, allow positional result reads. */
    EXEC_REJECTED /* No retry on this fid. */
} ExecPhase;

struct StyxRpcNodeService
{
    StyxRpcNodeInvoke invoke;               /* Borrowed callback, used exactly once. */
    void *context;                         /* Borrowed host context. */
    ExecPhase phase;                        /* State owned by the dispatcher. */
    unsigned char header[NODE_EXEC_HEADER]; /* Counted path and argument size. */
    unsigned char *body;                    /* Owned NUL-terminated path, then in-place bytes. */
    size_t path_size;                       /* Body bytes occupied by path including NUL. */
    size_t body_size;                       /* Total body size after header validation. */
    size_t received;                        /* Sequential bytes accepted including header. */
    size_t result_size;                     /* Confirmed output prefix from the callback. */
};


/* EN: Decode a big-endian count from one complete header word.
 * RU: Декодировать счётчик из полного слова заголовка старшим байтом вперёд. */
static uint32_t count(const unsigned char *word)
{
    uint32_t value = 0;
    for (size_t i = 0; i < NODE_EXEC_WORD; ++i)
    {
        value = (value << 8) | word[i];
    }
    return value;
}


/* EN: Reserve a bounded body only after the full header was received.
 * RU: Выделить буфер в пределах лимита после получения полного заголовка. */
static int prepare(StyxRpcNodeService *file)
{
    uint32_t path = count(file->header);
    uint32_t size = count(file->header + NODE_EXEC_WORD);
    if (path < 2 || path > STYXRPC_NODE_PATH_MAX + 1 || size > STYXRPC_NODE_BUFFER_MAX)
    {
        return 0;
    }
    file->path_size = path;
    file->body_size = path + size;
    file->body = malloc(file->body_size);
    return file->body != NULL;
}


/* EN/RU: See node_service.h. / См. node_service.h. */
StyxRpcNodeService *styxrpc_node_service_open(StyxRpcNodeInvoke invoke, void *context)
{
    StyxRpcNodeService *file = invoke ? calloc(1, sizeof(*file)) : NULL;
    if (file)
    {
        file->invoke = invoke;
        file->context = context;
    }
    return file;
}


/* EN: Accumulate exactly one sequential request and invoke after its last byte.
 * RU: Собрать один последовательный запрос и выполнить по последнему байту. */
int styxrpc_node_service_write(StyxRpcNodeService *file, uint64_t offset, const void *input, size_t size, size_t *used)
{
    const unsigned char *bytes = input;
    size_t remaining = size;
    if (!file || !used || (!input && size) || file->phase != EXEC_REQUEST)
    {
        return 0;
    }
    if (offset != file->received || !size)
    {
        file->phase = EXEC_REJECTED;
        return 0;
    }
    if (file->received < NODE_EXEC_HEADER)
    {
        size_t needed = NODE_EXEC_HEADER - file->received;
        size_t copy = remaining < needed ? remaining : needed;
        memcpy(file->header + file->received, bytes, copy);
        file->received += copy;
        bytes += copy;
        remaining -= copy;
        if (file->received == NODE_EXEC_HEADER && !prepare(file))
        {
            file->phase = EXEC_REJECTED;
            return 0;
        }
    }
    if (file->body)
    {
        size_t at = file->received - NODE_EXEC_HEADER;
        if (remaining > file->body_size - at)
        {
            file->phase = EXEC_REJECTED;
            return 0;
        }
        memcpy(file->body + at, bytes, remaining);
        file->received += remaining;
        if (at + remaining == file->body_size)
        {
            file->phase = EXEC_REJECTED;
            const char *path = (const char *)file->body;
            size_t extent = file->body_size - file->path_size;
            if (path[file->path_size - 1] || memchr(path, 0, file->path_size - 1) ||
                file->invoke(file->context, path, file->body + file->path_size, extent, &file->result_size) != 1 ||
                file->result_size > extent)
            {
                return 0;
            }
            file->phase = EXEC_RESULT;
        }
    }
    *used = size;
    return 1;
}


/* EN: Copy the positional result prefix from a completed invocation.
 * RU: Скопировать результат завершённого вызова с указанного смещения. */
int styxrpc_node_service_read(StyxRpcNodeService *file, uint64_t offset, void *bytes, size_t size, size_t *used)
{
    if (!file || !used || (!bytes && size) || file->phase != EXEC_RESULT)
    {
        return 0;
    }
    *used = 0;
    if (offset < file->result_size && size)
    {
        size_t remaining = file->result_size - (size_t)offset;
        *used = size < remaining ? size : remaining;
        memcpy(bytes, file->body + file->path_size + (size_t)offset, *used);
    }
    return 1;
}


/* EN: Destroy the exact fid state without executing a partial request.
 * RU: Освободить точное состояние fid без выполнения недописанного запроса. */
void styxrpc_node_service_close(StyxRpcNodeService *file)
{
    if (file)
    {
        free(file->body);
        free(file);
    }
}
