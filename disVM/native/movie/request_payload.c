/* Copyright (c) 2026 bhgv
 * EN: Shared bounded resource request encoding/validation for ordinary Styx and typed Unibus.
 * RU: Общее ограниченное кодирование/проверка запросов для обычного Styx и typed Unibus.
 */
#include "request_payload.h"
#include <string.h>


/* EN/RU: Bound a borrowed name scan; MOVIE_PATH signals excessive length, NULL means empty.
 * Ограничить просмотр заимствованного имени; MOVIE_PATH — превышение, NULL — пустое имя.
 */
static size_t length(const char *name)
{
    size_t size = 0; /* Counted name bytes, never includes a wire NUL. */
    while (name && size < MOVIE_PATH && name[size])
    {
        size++;
    }
    return size;
}


/* EN/RU: See request_payload.h; validate all extents before publishing any request byte.
 * См. request_payload.h; проверить все размеры до публикации байтов запроса.
 */
int movie_request_payload(void *out, size_t capacity, size_t *size, unsigned purpose,
    const char *reference, const char *base)
{
    size_t rn = length(reference); /* Exact counted reference bytes. */
    size_t bn = length(base); /* Exact counted base bytes. */
    if (!out || !size || !rn || rn >= MOVIE_PATH || bn >= MOVIE_PATH ||
        (purpose != MOVIE_CODE && purpose != MOVIE_DATA) || capacity < 6 + rn + bn)
    {
        return 0;
    }
    unsigned char *bytes = out; /* Caller-owned output after complete validation. */
    bytes[0] = MOVIE_REQUEST;
    bytes[1] = (unsigned char)purpose;
    bytes[2] = (unsigned char)rn;
    bytes[3] = (unsigned char)(rn >> 8);
    bytes[4] = (unsigned char)bn;
    bytes[5] = (unsigned char)(bn >> 8);
    memcpy(bytes + 6, reference, rn);
    if (bn)
    {
        memcpy(bytes + 6 + rn, base, bn);
    }
    *size = 6 + rn + bn;
    return 1;
}


/* EN/RU: See request_payload.h; input and all outputs are borrowed and nonoverlapping.
 * См. request_payload.h; input и outputs заимствованы и не перекрываются.
 */
int movie_request_parse(const void *input, size_t size, unsigned *purpose, char *reference, char *base)
{
    const unsigned char *bytes = input; /* Borrowed exact body, never a native wire struct. */
    if (!bytes || size < 6 || !purpose || !reference || !base || bytes[0] != MOVIE_REQUEST ||
        (bytes[1] != MOVIE_CODE && bytes[1] != MOVIE_DATA))
    {
        return 0;
    }
    size_t rn = bytes[2] | (size_t)bytes[3] << 8; /* Explicit little-endian wire count. */
    size_t bn = bytes[4] | (size_t)bytes[5] << 8; /* Explicit little-endian wire count. */
    if (!rn || rn >= MOVIE_PATH || bn >= MOVIE_PATH || size != 6 + rn + bn || memchr(bytes + 6, 0, rn + bn))
    {
        return 0;
    }
    memcpy(reference, bytes + 6, rn);
    reference[rn] = 0;
    memcpy(base, bytes + 6 + rn, bn);
    base[bn] = 0;
    *purpose = bytes[1];
    return 1;
}
