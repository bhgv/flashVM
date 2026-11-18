/* Copyright (c) 2026 bhgv
 * EN: Independent malformed/valid offer payload oracles and native64 generation boundary checks.
 * RU: Независимые valid/malformed offer payload oracles и native64 generation boundaries.
 */
#include "announcement.h"
#include <stdio.h>
#include <stdlib.h>


/* EN/RU: Explicit fixture check active in release builds; why borrowed until exit/return.
 * Явная fixture check в release builds, why заимствован до exit/return.
 */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "offer payload: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Refused exact body must preserve both caller outputs, including NULL/truncated input.
 * Отказ точного body сохраняет оба caller outputs, включая NULL/truncated input.
 */
static void refuse(const void *bytes, size_t size)
{
    unsigned state = 99; /* Native output sentinel. */
    unsigned long long generation = 99; /* Native output sentinel. */
    check(!movie_offer_parse(bytes, size, &state, &generation) && state == 99 && generation == 99,
        "invalid payload publishes no partial native outputs");
}


/* EN/RU: Check independent canonical small vectors and upper signed64 identity through existing frame codec.
 * Независимые canonical small vectors и верхняя signed64 identity через existing frame codec.
 */
int main(void)
{
    static const unsigned char ready[] = {1, 0x81}; /* Independent READY, positive one, final integer bit. */
    static const unsigned char closed[] = {2, 0x82}; /* Independent CLOSED, positive two. */
    static const unsigned char invalid[][3] = { /* Unknown state, zero, negative, redundant and trailing. */
        {3, 0x81, 0}, {1, 0x80, 0}, {1, 0xc1, 0}, {1, 0, 0x81}, {1, 0x81, 0x80}
    };
    unsigned state = 0; /* Caller-owned parsed kind. */
    unsigned long long generation = 0; /* Caller-owned parsed generation. */
    check(movie_offer_parse(ready, sizeof(ready), &state, &generation) && state == 1 && generation == 1 &&
        movie_offer_parse(closed, sizeof(closed), &state, &generation) && state == 2 && generation == 2,
        "independent wire vectors");
    refuse(NULL, 0);
    refuse(NULL, 2);
    refuse(ready, 0);
    refuse(ready, 1);
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++)
    {
        refuse(invalid[i], i < 3 ? 2 : 3);
    }
    unsigned char frame[MOVIE_OFFER_FRAME]; /* Existing checked record framing. */
    size_t size; /* Exact encoded bytes. */
    size_t used; /* Exact parsed frame extent. */
    FVM_SRecord record; /* Borrowed body into frame. */
    check(movie_offer_record(23, MOVIE_OFFER_READY, MOVIE_OFFER_GENERATION_MAX, frame, sizeof(frame), &size) &&
        fvm_srecord_read(frame, size, sizeof(frame), &record, &used) == FVM_S_OK && used == size &&
        movie_offer_parse(record.payload, record.size, &state, &generation) &&
        state == MOVIE_OFFER_READY && generation == MOVIE_OFFER_GENERATION_MAX,
        "positive native64 maximum");
    unsigned char above[1 + FVM_SRECORD_INTEGER_MAX]; /* Well-formed integer beyond admitted signed64 generations. */
    above[0] = MOVIE_OFFER_READY;
    FVM_SInteger number = {0, 0, 0}; /* Explicit portable integer limbs. */
    number.high = 0x80000000UL;
    check(fvm_sinteger_write(&number, above + 1, sizeof(above) - 1, &size) == FVM_S_OK, "encode wider identity");
    refuse(above, size + 1);
    check(!movie_offer_parse(ready, sizeof(ready), NULL, &generation) &&
        !movie_offer_parse(ready, sizeof(ready), &state, NULL), "required native outputs");
    return 0;
}
