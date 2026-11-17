/* Copyright (c) 2026 bhgv
 * EN: Native push readiness, complete-record ownership, partial acknowledgements and teardown.
 * RU: Native push readiness, complete-record ownership, partial acknowledgements и teardown.
 */
#include "writer.h"
#include "service.h"
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Explicit release-build invariant; valid/diagnostic borrowed, exits on failure.
 * Явный release-build invariant; valid/diagnostic borrowed, exit при отказе. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "movie writer: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Native acknowledgement fixture owns captured bytes and injection state.
 * Native acknowledgement fixture владеет captured bytes и injection state. */
typedef struct Sink
{
    unsigned char bytes[8192]; /* Confirmed byte sequence, not a host file. */
    size_t used; /* Verified confirmed prefix. */
    size_t limit; /* Maximum progress per callback. */
    unsigned calls; /* Exact callback admissions. */
    int fault; /*1 AGAIN,2 uncertain,3 overflow,4 failure,5 zero,6 EOF,7 flags,8 no PREFIX,9 status,10 stop,11 status/stop. */
} Sink;


/* EN/RU: Acknowledge one borrowed prefix, optionally injecting a precise failure/AGAIN.
 * Подтвердить borrowed prefix с optional precise failure/AGAIN; no transport/VM. */
static void writebytes(void *state, const void *bytes, size_t size, DVM_IoResult *out)
{
    Sink *sink = state;
    size_t count = size < sink->limit ? size : sink->limit;
    check(sink->used+count <= sizeof(sink->bytes), "fixture capacity");
    memcpy(sink->bytes+sink->used, bytes, count);
    sink->used += count;
    sink->calls++;
    memset(out, 0, sizeof(*out));
    out->status = FVM_OK;
    out->flags = DVM_IO_PREFIX;
    out->count = count;
    if (sink->fault == 1)
    {
        out->status = FVM_ERROR;
        out->stop = DVM_IO_AGAIN;
    }
    else if (sink->fault == 2)
    {
        out->flags |= DVM_IO_UNCERTAIN;
    }
    else if (sink->fault == 3)
    {
        out->count = size+1;
    }
    else if (sink->fault == 4)
    {
        out->status = FVM_ERROR;
        out->stop = DVM_IO_FAILURE;
    }
    else if (sink->fault == 6)
    {
        out->stop = DVM_IO_EOF;
    }
    else if (sink->fault == 7)
    {
        out->flags |= 0x8000U;
    }
    else if (sink->fault == 8)
    {
        out->flags = 0;
    }
    else if (sink->fault == 9)
    {
        out->status = (FVM_Result)100;
    }
    else if (sink->fault == 10)
    {
        out->stop = (DVM_IoStop)-1;
    }
    else if (sink->fault == 11)
    {
        out->status = FVM_ERROR;
    }
}


/* EN/RU: Drain exactly one announcement with bounded positive steps; no sleeps or guest cooperation.
 * Считать одно announcement за bounded positive steps, без sleeps/guest cooperation. */
static void drain(StyxMovieWriter *writer, Sink *sink)
{
    for (unsigned i = 0; i < MOVIE_OFFER_FRAME*2; i++)
    {
        DVM_IoResult out = styxmoviewriterstep(writer, writebytes, sink);
        check(out.status == FVM_OK && out.count && !out.stop, "drain progress");
        if (out.flags & DVM_IO_RECORD)
        {
            return;
        }
    }
    check(0, "drain bound");
}


/* EN/RU: Overflow a bounded queue; encoded state must be the latest admitted generation.
 * Переполнить bounded queue; encoded state соответствует latest admitted generation. */
static void overflow(DVM_Provider *service)
{
    DVM_MovieOffer *source = dvm_movie_offer_create();
    StyxEvents *events = styxmovieofferwatch(source, 13);
    check(events != NULL, "overflow setup");
    for (uint64_t generation = 0; generation < 100; generation++)
    {
        check(dvm_movie_offer_bind(source, generation, service) == generation+1, "overflow source replacement");
    }
    unsigned char expected[MOVIE_OFFER_FRAME*2], bytes[MOVIE_OFFER_FRAME*2];
    size_t size, count;
    check(styxmovieofferframe(source, 13, expected, sizeof(expected), &size) == FVM_OK, "overflow latest snapshot");
    unsigned drained = 0;
    while (styxeventsread(events, bytes, sizeof(bytes), &count) == STYX_EVENT_RECORD)
    {
        check(++drained <= 33 && count == size && !memcmp(expected, bytes, size), "overflow emitted stale/unbounded records");
    }
    check(drained && dvm_movie_offer_close(source, 100), "overflow latest state lost");
    styxeventsfree(events);
    dvm_movie_offer_release(source);
}


/* EN/RU: Publish through independent source lifetime while other native callers detach writers.
 * Публиковать под independent source lifetime при concurrent writer detach. */
typedef struct Work
{
    DVM_MovieOffer *source; /* Borrowed through joined native worker. */
    DVM_Provider *service; /* Borrowed through joined native worker. */
} Work;


/* EN/RU: Generate native invalidations with no transport, VM entry or guest coordination.
 * Генерировать native invalidations без transport/VM/guest coordination. */
static void *publish(void *state)
{
    Work *work = state;
    for (uint64_t generation = 0; generation < 1000; generation++)
    {
        check(dvm_movie_offer_bind(work->source, generation, work->service) == generation+1,
            "concurrent source generation");
    }
    check(dvm_movie_offer_close(work->source, 1000), "concurrent source close");
    return NULL;
}


/* EN/RU: Detach active native callbacks repeatedly; joining only the test producer proves cleanup.
 * Многократно detach active callbacks; join только test producer проверяет cleanup. */
static void detach(DVM_Provider *service)
{
    DVM_MovieOffer *source = dvm_movie_offer_create();
    Work work = {source, service};
    pthread_t worker;
    check(source && !pthread_create(&worker, NULL, publish, &work), "concurrent producer");
    for (unsigned i = 0; i < 200; i++)
    {
        StyxMovieWriter *writer = styxmoviewriter(source, 17);
        check(writer != NULL, "concurrent subscription");
        Sink sink = {{0}, 0, 3, 0, 0};
        DVM_IoResult out = styxmoviewriterstep(writer, writebytes, &sink);
        check(out.stop == DVM_IO_AGAIN || out.count, "concurrent read");
        styxmoviewriterfree(writer);
    }
    check(!pthread_join(worker, NULL), "producer join");
    dvm_movie_offer_release(source);
}


/* EN/RU: Exercise source fanout, initial staging, partially sent old READY and sticky failure.
 * Проверить source fanout, initial staging, partial old READY и sticky failure. */
int main(void)
{
    DVM_Namespace *group = dvm_namespace_create("/movie-writer-no-native-fallback", "/", "/");
    DVM_Provider *service = group ? styxmovieservice(group) : NULL;
    DVM_MovieOffer *source = dvm_movie_offer_create();
    StyxMovieWriter *writer = styxmoviewriter(source, 7);
    StyxEvents *other = styxmovieofferwatch(source, 9);
    check(service && source && writer && other && !styxmoviewriter(source, 0), "initial setup/tag refusal");
    Sink sink = {{0}, 0, 1, 0, 0};
    DVM_IoResult out = styxmoviewriterstep(writer, writebytes, &sink);
    check(out.stop == DVM_IO_AGAIN && !out.count && !sink.calls, "empty became EOF or called transport");
    check(dvm_movie_offer_stage(source, service), "stage");
    struct pollfd readiness = {styxmoviewriterfd(writer), POLLIN, 0};
    check(!poll(&readiness, 1, 0), "unpublished stage signalled");
    check(dvm_movie_offer_commit(source) == 1 && poll(&readiness, 1, 0) == 1, "native commit did not signal");
    unsigned char expected[MOVIE_OFFER_FRAME*2];
    size_t size;
    check(styxmovieofferframe(source, 7, expected, sizeof(expected), &size) == FVM_OK, "expected READY");
    out = styxmoviewriterstep(writer, writebytes, &sink);
    check(out.count == 1 && styxmoviewriterpending(writer) == size-1, "partial prefix missing");
    sink.limit = 2;
    sink.fault = 1;
    out = styxmoviewriterstep(writer, writebytes, &sink);
    check(out.stop == DVM_IO_AGAIN && out.count == 2 && styxmoviewriterpending(writer) == size-3,
        "partial AGAIN lost prefix");
    sink.limit = 0;
    sink.fault = 1;
    out = styxmoviewriterstep(writer, writebytes, &sink);
    check(out.stop == DVM_IO_AGAIN && !out.count && styxmoviewriterpending(writer) == size-3,
        "AGAIN consumed tail");
    check(dvm_movie_offer_close(source, 1), "close during partial READY");
    sink.limit = 3;
    sink.fault = 0;
    drain(writer, &sink);
    check(sink.used == size && !memcmp(sink.bytes, expected, size), "partial old READY interleaved/truncated");
    size_t closed_size;
    check(styxmovieofferframe(source, 7, expected, sizeof(expected), &closed_size) == FVM_OK, "expected CLOSED");
    drain(writer, &sink);
    check(sink.used == size+closed_size && !memcmp(sink.bytes+size, expected, closed_size), "CLOSED lost");
    unsigned calls = sink.calls;
    out = styxmoviewriterstep(writer, writebytes, &sink);
    check(out.status == FVM_OK && out.stop == DVM_IO_EOF && sink.calls == calls && styxmoviewriterfd(writer) == -1, "CLOSED not followed by EOF");
    unsigned char buffer[MOVIE_OFFER_FRAME*2];
    size_t count;
    check(styxeventsread(other, buffer, 1, &count) == STYX_EVENT_SMALL && !count &&
        styxeventsread(other, buffer, sizeof(buffer), &count) == STYX_EVENT_RECORD, "independent subscriber/head lost");
    for (int fault = 2; fault <= 11; fault++)
    {
        StyxMovieWriter *bad = styxmoviewriter(source, 11);
        check(bad != NULL, "failure fixture subscription");
        Sink broken = {{0}, 0, fault == 5 ? 0 : 1, 0, fault};
        out = styxmoviewriterstep(bad, writebytes, &broken);
        check(out.stop || (out.flags & DVM_IO_UNCERTAIN), "bad ACK accepted as progress");
        calls = broken.calls;
        broken.fault = 0;
        out = styxmoviewriterstep(bad, writebytes, &broken);
        check(out.stop == DVM_IO_FAILURE && broken.calls == calls && !styxmoviewriterpending(bad),
            "failed/uncertain tail replayed");
        styxmoviewriterfree(bad);
    }
    styxmoviewriterfree(writer);
    styxeventsfree(other);
    dvm_movie_offer_release(source);
    overflow(service);
    detach(service);
    dvm_provider_release(service);
    dvm_namespace_release(group);
    puts("PASS native movie outbox readiness, prefix preservation, fanout and sticky ACK refusal");
    return 0;
}
