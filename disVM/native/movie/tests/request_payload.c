/* Copyright (c) 2026 bhgv
 * EN: Independent resource request wire vectors and atomic refusal at path/count/capacity boundaries.
 * RU: Независимые wire-векторы запросов и атомарный отказ на границах path/count/capacity.
 */
#include "request_payload.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Stop on a violated wire contract in every build profile; why borrowed through return.
 * Завершить при нарушении wire-контракта в любом build profile; why заимствован до возврата.
 */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "resource request payload: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Require malformed payload refusal without changing reference/base/purpose outputs.
 * Проверить отказ malformed payload без изменения outputs reference/base/purpose.
 */
static void refuse(const void *bytes, size_t size)
{
    char reference[MOVIE_PATH] = "reference"; /* Output sentinel. */
    char base[MOVIE_PATH] = "base"; /* Output sentinel. */
    unsigned purpose = 99; /* Output sentinel. */
    check(!movie_request_parse(bytes, size, &purpose, reference, base) && purpose == 99 &&
        !strcmp(reference, "reference") && !strcmp(base, "base"), "invalid payload preserves outputs");
}


/* EN/RU: Verify explicit little-endian counts and intent, binary extents, max names and refusal atomicity.
 * Проверить little-endian counts/intent, точные размеры, максимальные имена и атомарность отказа.
 */
int main(void)
{
    unsigned char output[MOVIE_FRAME]; /* Encoded payload, never a native struct dump. */
    size_t size = 999; /* Output sentinel. */
    static const unsigned char expected[] = {1, 1, 2, 0, 2, 0, '/', 'x', '/', 'b'}; /* Independent wire vector. */
    check(movie_request_payload(output, sizeof(output), &size, MOVIE_CODE, "/x", "/b") &&
        size == sizeof(expected) && !memcmp(output, expected, size), "exact existing request layout");
    char reference[MOVIE_PATH]; /* Separate decoded reference. */
    char base[MOVIE_PATH]; /* Separate decoded base. */
    unsigned purpose = 0; /* Exact decoded explicit intent. */
    check(movie_request_parse(expected, sizeof(expected), &purpose, reference, base) &&
        purpose == MOVIE_CODE && !strcmp(reference, "/x") && !strcmp(base, "/b"), "independent vector decoded");
    memset(output, 77, sizeof(output));
    size = 999;
    check(!movie_request_payload(output, 9, &size, MOVIE_CODE, "/x", "/b") &&
        size == 999 && output[0] == 77 && output[9] == 77, "small output remains unchanged");
    check(!movie_request_payload(output, sizeof(output), &size, 99, "/x", NULL) &&
        !movie_request_payload(output, sizeof(output), &size, MOVIE_CODE, "", NULL) &&
        !movie_request_payload(output, sizeof(output), &size, MOVIE_CODE, NULL, NULL) &&
        size == 999 && output[0] == 77, "invalid intent/name before mutation");
    char longname[MOVIE_PATH + 1]; /* Native bounded-scan boundary, no truncation. */
    memset(longname, 'n', sizeof(longname));
    longname[MOVIE_PATH] = 0;
    check(!movie_request_payload(output, sizeof(output), &size, MOVIE_CODE, longname, NULL) &&
        !movie_request_payload(output, sizeof(output), &size, MOVIE_CODE, "/x", longname) &&
        size == 999 && output[0] == 77, "overlong names never truncated");
    longname[MOVIE_PATH - 1] = 0;
    check(movie_request_payload(output, sizeof(output), &size, MOVIE_DATA, longname, longname) &&
        size == 6 + 2 * (MOVIE_PATH - 1) && output[2] == 255 && output[3] == 15 &&
        movie_request_parse(output, size, &purpose, reference, base) && purpose == MOVIE_DATA &&
        strlen(reference) == MOVIE_PATH - 1 && !strcmp(reference, base), "max counted names and data intent");
    check(movie_request_payload(output, sizeof(output), &size, MOVIE_CODE, "\320\257", NULL) &&
        size == 8 && output[2] == 2 && output[4] == 0 &&
        movie_request_parse(output, size, &purpose, reference, base) && !strcmp(reference, "\320\257") && !*base,
        "counted UTF-8 bytes and nullable base");
    static const unsigned char invalid[][10] = { /* Independent malformed vectors. */
        {99, 1, 1, 0, 0, 0, 'x'}, {1, 99, 1, 0, 0, 0, 'x'},
        {1, 1, 0, 0, 0, 0}, {1, 1, 1, 16, 0, 0, 'x'},
        {1, 1, 1, 0, 0, 0, 0}, {1, 1, 1, 0, 1, 0, 'x', 0},
        {1, 1, 2, 0, 0, 0, 'x'}, {1, 1, 1, 0, 0, 0, 'x', 'y'}
    };
    static const size_t lengths[] = {7, 7, 6, 7, 7, 8, 7, 8}; /* Exact malformed extents. */
    for (size_t i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++)
    {
        refuse(invalid[i], lengths[i]);
    }
    refuse(NULL, 9);
    refuse(expected, 5);
    refuse(expected, (size_t)-1);
    puts("PASS existing resource request payload codec");
    return 0;
}
