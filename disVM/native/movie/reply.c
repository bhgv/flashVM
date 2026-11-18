/* Copyright (c) 2026 bhgv
 * EN: Resource reply validation and staged delivery, independent of transport and numeric S-tags.
 * RU: Проверка ответа ресурса и staging-доставка без зависимости от транспорта и числовых S-тегов.
 */
#include "reply.h"
#include <string.h>


/* EN/RU: Validate borrowed absolute virtual origin without changing its bytes; 1 valid/0 invalid.
 * Reject NUL, empty/dot/parent components and trailing separators; root itself remains valid.
 * Проверить absolute virtual origin без изменения байтов; запретить NUL/пустые/dot/parent компоненты.
 */
static int origin(const unsigned char *path, size_t size)
{
    if (!size || size >= MOVIE_PATH || path[0] != '/' || memchr(path, 0, size))
    {
        return 0;
    }
    if (size == 1)
    {
        return 1;
    }
    size_t first = 1; /* First byte of the current path component. */
    for (size_t at = 1; at <= size; at++)
    {
        if (at == size || path[at] == '/')
        {
            size_t length = at - first; /* Component length without separator. */
            if (!length || (length == 1 && path[first] == '.') ||
                (length == 2 && path[first] == '.' && path[first + 1] == '.'))
            {
                return 0;
            }
            first = at + 1;
        }
    }
    return 1;
}


/* EN/RU: See reply.h; all borrowed inputs are validated before state/output mutation.
 * См. reply.h; проверить все заимствованные inputs до изменения state/output.
 */
int movie_reply_init(MovieReply *reply, FVM_DesktopWrite write, void *state, char *resolved, size_t capacity)
{
    if (!reply || !write || !resolved || !capacity)
    {
        return 0;
    }
    memset(reply, 0, sizeof(*reply));
    reply->write = write;
    reply->state = state;
    reply->resolved = resolved;
    reply->capacity = capacity;
    reply->result = FVM_ERROR;
    return 1;
}


/* EN/RU: See reply.h; only valid data invokes the caller-owned native sink, once per accepted chunk.
 * См. reply.h; только корректные данные вызывают native sink caller, один раз на принятый чанк.
 */
int movie_reply_accept(MovieReply *reply, const void *bytes, size_t size)
{
    if (!reply)
    {
        return MOVIE_REPLY_FAILED;
    }
    if (reply->stage == -1)
    {
        return MOVIE_REPLY_FAILED;
    }
    const unsigned char *payload = bytes; /* Borrowed through this single validation/callback. */
    if (!payload || !size || reply->stage == 2 || !reply->write)
    {
        goto invalid;
    }
    unsigned kind = payload[0]; /* Existing resource protocol opcode, not a new S-tag. */
    if (kind == MOVIE_BEGIN && reply->stage == 0 && size > 1 &&
        size - 1 < reply->capacity && origin(payload + 1, size - 1))
    {
        reply->path_size = size - 1;
        memcpy(reply->path, payload + 1, reply->path_size);
        reply->path[reply->path_size] = 0;
        reply->stage = 1;
        return MOVIE_REPLY_MORE;
    }
    if (kind == MOVIE_BYTES && reply->stage == 1 && size > 1 && size - 1 <= MOVIE_CHUNK &&
        reply->bytes <= (size_t)-1 - (size - 1))
    {
        reply->result = reply->write(payload + 1, size - 1, reply->state);
        if (reply->result != FVM_OK)
        {
            reply->stage = -1;
            return MOVIE_REPLY_FAILED;
        }
        reply->bytes += size - 1;
        reply->result = FVM_ERROR;
        return MOVIE_REPLY_MORE;
    }
    if (kind == MOVIE_END && size == 2 && payload[1] == 0 && reply->stage == 1)
    {
        memcpy(reply->resolved, reply->path, reply->path_size + 1);
        reply->stage = 2;
        reply->result = FVM_OK;
        return MOVIE_REPLY_DONE;
    }
invalid:
    reply->stage = -1;
    reply->result = FVM_ERROR;
    return MOVIE_REPLY_FAILED;
}


/* EN/RU: See reply.h; a partial stream never reports successful resource acquisition.
 * См. reply.h; частичный поток никогда не означает успешного получения ресурса.
 */
FVM_Result movie_reply_result(const MovieReply *reply)
{
    if (reply && reply->stage == 2 && reply->result == FVM_OK)
    {
        return FVM_OK;
    }
    return reply && reply->stage == -1 && reply->result != FVM_OK ? reply->result : FVM_ERROR;
}
