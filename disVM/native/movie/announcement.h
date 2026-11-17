/* Copyright (c) 2026 bhgv
 * EN: Scoped movie endpoint announcement; no address, username or capability pointer on wire.
 * RU: Scoped объявление movie endpoint без адреса, username и capability pointer на wire.
 */
#ifndef STYX_MOVIE_ANNOUNCEMENT_H
#define STYX_MOVIE_ANNOUNCEMENT_H
#include "protocol.h"
#define MOVIE_OFFER_TYPE "flashvm.movie.offer" /* Binding identity on the already granted control stream. */
#define MOVIE_OFFER_VERSION 1UL /* Payload: ready/closed byte then one canonical positive generation. */
#define MOVIE_OFFER_READY 1U /* Current logical service root is available. */
#define MOVIE_OFFER_CLOSED 2U /* Sticky retirement, not permission to dial another endpoint. */
#define MOVIE_OFFER_GENERATION_MAX 0x7fffffffffffffffULL /* Positive signed64 range; upper identities reserved. */
#define MOVIE_OFFER_FRAME 256U /* Bound complete declaration and announcement records individually. */


/* EN/RU: Encode tag0 declaration for a caller-selected unused tag;1/0, output borrowed.
 * Сформировать tag0 declaration для выбранного свободного tag;1/0, output borrowed. */
int movie_offer_binding(unsigned long tag, void *out, size_t capacity, size_t *size);


/* EN/RU: Encode ready/closed generation into one record;1/0, native limbs are not serialized structs.
 * Сформировать ready/closed generation record;1/0, native struct не передаётся. */
int movie_offer_record(unsigned long tag, unsigned state, unsigned long long generation,
    void *out, size_t capacity, size_t *size);
#endif
