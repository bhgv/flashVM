/* Copyright (c) 2026 bhgv
 * EN: Existing resource request payload codec, independent of S-record tags and VFS execution.
 * RU: Кодек существующего payload запроса ресурса без S-тегов и исполнения операций VFS.
 */
#ifndef STYX_MOVIE_REQUEST_PAYLOAD_H
#define STYX_MOVIE_REQUEST_PAYLOAD_H
#include "protocol.h"
#ifdef __cplusplus
extern "C" {
#endif


/* EN/RU: Encode REQUEST/purpose/u16 lengths/reference/base into borrowed out; 1/0, no I/O.
 * Purpose is MOVIE_CODE/DATA; reference nonempty, both names shorter than MOVIE_PATH, base nullable.
 * Outputs unchanged on refusal; names/out/size must not overlap. No normalization or permission.
 * Кодировать существующий запрос в out, 1/0 без I/O; outputs неизменны при отказе, без overlap.
 */
int movie_request_payload(void *out, size_t capacity, size_t *size, unsigned purpose,
    const char *reference, const char *base);


/* EN/RU: Decode one exact borrowed payload into separate MOVIE_PATH buffers and purpose; 1/0.
 * Reject wrong opcode/purpose/counts/NUL/trailing bytes before changing outputs. No VFS lookup.
 * Декодировать точный payload в отдельные буферы MOVIE_PATH, 1/0; refusal сохраняет outputs.
 */
int movie_request_parse(const void *bytes, size_t size, unsigned *purpose, char *reference, char *base);
#ifdef __cplusplus
}
#endif
#endif
