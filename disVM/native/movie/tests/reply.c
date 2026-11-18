/* Copyright (c) 2026 bhgv
 * EN: Resource reply state tests with independent payloads, native staging and explicit terminal failures.
 * RU: Проверки ответа ресурса с независимыми payloads, native staging и явными terminal failures.
 */
#include "reply.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Native fixture collector, owned by main through synchronous sink calls.
 * Native-сборщик fixture, main владеет им до завершения синхронных sink calls.
 */
typedef struct Sink
{
    unsigned calls; /* Actual sink invocations, including refused calls. */
    size_t size; /* Accepted staged bytes. */
    unsigned char bytes[MOVIE_CHUNK + 16]; /* Bounded independent staging storage. */
    FVM_Result refusal; /* Nonzero injected result, never accepted as a successful transfer. */
} Sink;


/* EN/RU: Stop at a violated contract in every build profile; why borrowed through return.
 * Завершить при нарушении контракта в любом build profile, why заимствован до возврата.
 */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "movie reply: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Copy valid staged bytes or return injected refusal; borrowed bytes/context, no VM/I/O.
 * Скопировать staging либо вернуть заданный отказ, bytes/context заимствованы, без VM/I/O.
 */
static FVM_Result writebytes(const void *bytes, size_t size, void *opaque)
{
    Sink *sink = opaque; /* Main-owned fixture. */
    sink->calls++;
    if (sink->refusal != FVM_OK)
    {
        return sink->refusal;
    }
    check(size <= sizeof(sink->bytes) - sink->size, "bounded staging sink");
    memcpy(sink->bytes + sink->size, bytes, size);
    sink->size += size;
    return FVM_OK;
}


/* EN/RU: Prepare a fresh borrowed test state; output sentinel must survive until successful END.
 * Подготовить новое заимствованное состояние; sentinel output живёт до успешного END.
 */
static void start(MovieReply *reply, Sink *sink, char *resolved, size_t capacity)
{
    memset(sink, 0, sizeof(*sink));
    strcpy(resolved, "unchanged");
    check(movie_reply_init(reply, writebytes, sink, resolved, capacity) &&
        movie_reply_result(reply) == FVM_ERROR && !strcmp(resolved, "unchanged"), "initial pending state");
}


/* EN/RU: Refuse one malformed first payload and all subsequent input, without invoking the sink.
 * Отклонить первый malformed payload и все следующие, не вызывая sink.
 */
static void badfirst(const void *payload, size_t size)
{
    MovieReply reply; /* Single-native-owner parser state. */
    Sink sink; /* Fresh independent collector. */
    char resolved[64]; /* Sentinel output. */
    start(&reply, &sink, resolved, sizeof(resolved));
    static const unsigned char begin[] = {2, '/', 'o', 'k'}; /* Independent valid later input. */
    check(movie_reply_accept(&reply, payload, size) == MOVIE_REPLY_FAILED &&
        movie_reply_accept(&reply, begin, sizeof(begin)) == MOVIE_REPLY_FAILED &&
        !sink.calls && movie_reply_result(&reply) != FVM_OK && !strcmp(resolved, "unchanged"),
        "malformed first input is sticky without sink effects");
}


/* EN/RU: Validate ordering, exact binary chunks, canonical paths, staged completion and sink error lifetime.
 * Проверить порядок, точные binary-чанки, canonical paths, staging completion и сохранение sink error.
 */
int main(void)
{
    MovieReply reply; /* Native request state reused only after explicit init. */
    Sink sink; /* Independent native collector. */
    char resolved[MOVIE_PATH + 2]; /* Larger than private path bound to test that bound independently. */
    static const unsigned char begin[] = {2, '/', 'x', '/', 'y'}; /* Valid canonical absolute origin. */
    static const unsigned char chunk[] = {3, 0, 128, 255, 0}; /* Exact binary payload. */
    static const unsigned char end[] = {4, 0}; /* Existing success opcode/status. */
    MovieReply uninitialized = {0}; /* Zero storage has no initialized successful request. */
    check(movie_reply_result(&uninitialized) == FVM_ERROR, "zero state is not completed acquisition");
    start(&reply, &sink, resolved, sizeof(resolved));
    check(!movie_reply_init(&reply, NULL, &sink, resolved, sizeof(resolved)) &&
        !movie_reply_init(NULL, writebytes, &sink, resolved, sizeof(resolved)) &&
        movie_reply_accept(NULL, end, sizeof(end)) == MOVIE_REPLY_FAILED &&
        movie_reply_result(NULL) == FVM_ERROR, "invalid setup/query arguments");
    check(movie_reply_accept(&reply, begin, sizeof(begin)) == MOVIE_REPLY_MORE &&
        movie_reply_accept(&reply, chunk, sizeof(chunk)) == MOVIE_REPLY_MORE &&
        movie_reply_accept(&reply, chunk, sizeof(chunk)) == MOVIE_REPLY_MORE &&
        !strcmp(resolved, "unchanged") && movie_reply_result(&reply) != FVM_OK &&
        sink.calls == 2 && sink.size == 8 && !memcmp(sink.bytes, chunk + 1, 4) &&
        !memcmp(sink.bytes + 4, chunk + 1, 4), "binary chunks staged without premature completion");
    check(movie_reply_accept(&reply, end, sizeof(end)) == MOVIE_REPLY_DONE &&
        movie_reply_result(&reply) == FVM_OK && !strcmp(resolved, "/x/y"), "END publishes origin and success");
    check(movie_reply_accept(&reply, chunk, sizeof(chunk)) == MOVIE_REPLY_FAILED && sink.calls == 2 &&
        movie_reply_result(&reply) != FVM_OK, "records after END cannot be replayed");
    static const unsigned char invalid[][6] = { /* Independent malformed payloads, explicit extents below. */
        {3, 1}, {4, 0}, {4, 1}, {4, 2}, {99}, {2},
        {2, 'x'}, {2, '/', 0}, {2, '/', '/'}, {2, '/', '.'},
        {2, '/', '.', '.'}, {2, '/', 'x', '/'}, {2, '/', '.', '/', 'x'},
        {2, '/', 'x', '/', '.', '.'}
    };
    static const size_t lengths[] = {2, 2, 2, 2, 1, 1, 2, 3, 3, 3, 4, 4, 5, 6};
    for (size_t i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++)
    {
        badfirst(invalid[i], lengths[i]);
    }
    badfirst(NULL, 1);
    badfirst(end, 0);
    start(&reply, &sink, resolved, sizeof(resolved));
    check(movie_reply_accept(&reply, begin, sizeof(begin)) == MOVIE_REPLY_MORE &&
        movie_reply_accept(&reply, begin, sizeof(begin)) == MOVIE_REPLY_FAILED && !sink.calls,
        "repeated BEGIN refuses before data");
    start(&reply, &sink, resolved, sizeof(resolved));
    sink.refusal = FVM_INVALID_ARGUMENT;
    check(movie_reply_accept(&reply, begin, sizeof(begin)) == MOVIE_REPLY_MORE &&
        movie_reply_accept(&reply, chunk, sizeof(chunk)) == MOVIE_REPLY_FAILED &&
        movie_reply_accept(&reply, end, sizeof(end)) == MOVIE_REPLY_FAILED && sink.calls == 1 &&
        movie_reply_result(&reply) == FVM_INVALID_ARGUMENT && !strcmp(resolved, "unchanged"),
        "sink refusal preserved without retry or origin publication");
    unsigned char large[MOVIE_CHUNK + 2]; /* One byte beyond protocol chunk bound. */
    memset(large, 7, sizeof(large));
    large[0] = 3;
    start(&reply, &sink, resolved, sizeof(resolved));
    check(movie_reply_accept(&reply, begin, sizeof(begin)) == MOVIE_REPLY_MORE &&
        movie_reply_accept(&reply, large, sizeof(large)) == MOVIE_REPLY_FAILED && !sink.calls,
        "oversized chunk refuses before sink");
    start(&reply, &sink, resolved, sizeof(resolved));
    check(movie_reply_accept(&reply, begin, sizeof(begin)) == MOVIE_REPLY_MORE &&
        movie_reply_accept(&reply, large, sizeof(large) - 1) == MOVIE_REPLY_MORE && sink.size == MOVIE_CHUNK,
        "maximum chunk accepted");
    start(&reply, &sink, resolved, sizeof(resolved));
    check(movie_reply_accept(&reply, begin, sizeof(begin)) == MOVIE_REPLY_MORE, "overflow fixture BEGIN");
    reply.bytes = (size_t)-2; /* Inject unreachable-scale boundary without allocating a huge resource. */
    check(movie_reply_accept(&reply, chunk, sizeof(chunk)) == MOVIE_REPLY_FAILED && !sink.calls,
        "size counter overflow refused before sink");
    start(&reply, &sink, resolved, 5);
    check(movie_reply_accept(&reply, begin, sizeof(begin)) == MOVIE_REPLY_MORE &&
        movie_reply_accept(&reply, end, sizeof(end)) == MOVIE_REPLY_DONE, "exact output capacity");
    start(&reply, &sink, resolved, 4);
    check(movie_reply_accept(&reply, begin, sizeof(begin)) == MOVIE_REPLY_FAILED &&
        !strcmp(resolved, "unchanged"), "small origin output preserves caller bytes");
    unsigned char path[MOVIE_PATH + 1]; /* Boundary-sized BEGIN payload, independent bytes. */
    memset(path, 'a', sizeof(path));
    path[0] = 2;
    path[1] = '/';
    start(&reply, &sink, resolved, sizeof(resolved));
    check(movie_reply_accept(&reply, path, MOVIE_PATH) == MOVIE_REPLY_MORE &&
        movie_reply_accept(&reply, end, sizeof(end)) == MOVIE_REPLY_DONE &&
        strlen(resolved) == MOVIE_PATH - 1 && !sink.calls, "maximum origin and empty resource");
    start(&reply, &sink, resolved, sizeof(resolved));
    check(movie_reply_accept(&reply, path, sizeof(path)) == MOVIE_REPLY_FAILED &&
        !strcmp(resolved, "unchanged"), "private path bound holds even with larger caller output");
    static const unsigned char emptychunk[] = {3}; /* Empty BYTES is not an END marker. */
    start(&reply, &sink, resolved, sizeof(resolved));
    check(movie_reply_accept(&reply, begin, sizeof(begin)) == MOVIE_REPLY_MORE &&
        movie_reply_accept(&reply, emptychunk, sizeof(emptychunk)) == MOVIE_REPLY_FAILED && !sink.calls,
        "empty chunk cannot signal EOF");
    puts("PASS shared resource reply state and staged completion");
    return 0;
}
