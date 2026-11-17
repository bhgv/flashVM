/* Copyright (c) 2026 bhgv
 * EN: Dedicated read-only movie service over one opened Styx byte stream.
 * RU: Read-only movie service через один открытый Styx byte stream.
 */
#ifndef STYX_MOVIE_PROTOCOL_H
#define STYX_MOVIE_PROTOCOL_H
#include "flashvm/sbindings.h"
#define MOVIE_TAG 1UL /* Dedicated direction-local service tag after tag-zero declaration. */
#define MOVIE_VERSION 1UL /* Service schema, independent of 9P and FlashVM ABI versions. */
#define MOVIE_TYPE "flashvm.movie" /* Immutable type identity negotiated on each opened stream. */
#define MOVIE_PATH 4096U /* Counted UTF-8 path capacity including local trailing NUL. */
#define MOVIE_FRAME 8320U /* Complete record capacity for two paths plus framing. */
#define MOVIE_CHUNK 2048U /* Bounded binary resource chunk, independent of 9P msize. */
#define MOVIE_REQUEST 1U /* Purpose byte, two little-endian u16 lengths, reference and base. */
#define MOVIE_BEGIN 2U /* Canonical absolute virtual origin, counted by record extent. */
#define MOVIE_BYTES 3U /* Opaque nonempty resource bytes. */
#define MOVIE_END 4U /* One byte: zero acknowledged success, one acquisition failure. */
#define MOVIE_CODE 1U /* Provider-first executable movie intent; never filename inference. */
#define MOVIE_DATA 2U /* Ordinary namespace data intent. */


/* EN/RU: Write one service record into borrowed output;1/0, no allocation or VM entry.
 * Записать service record в borrowed output;1/0, без allocation/VM entry. */
int movie_record(unsigned char *out, size_t *size, unsigned kind, const void *bytes, size_t count);


/* EN/RU: Encode initial tag-zero declaration into output;1/0, no state retained.
 * Записать начальное объявление tag0 в output;1/0, state не удерживается. */
int movie_binding(unsigned char *out, size_t *size);


/* EN/RU: Verify exactly the dedicated service binding; borrowed table,1/0, no I/O.
 * Проверить binding dedicated service; borrowed table,1/0, без I/O. */
int movie_bound(const FVM_SBindings *bindings);
#endif
