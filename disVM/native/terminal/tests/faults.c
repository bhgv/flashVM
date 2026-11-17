/* Copyright (c) 2026 bhgv
 * EN: Deterministic worker fault injection, framing bounds and uncertain-output no-replay checks.
 * RU: Детерминированные worker fault injection, framing bounds и no-replay при uncertain output.
 */
#include "../internal.h"
#include <stdio.h>
static unsigned char input[512]; /* Immutable fixture bytes after worker creation. */
static size_t input_size; /* Total supplied input bytes. */
static size_t input_at; /* RX worker-only cursor. */
static unsigned char output[512]; /* TX worker-only confirmed bytes. */
static size_t output_size; /* TX worker-only count, inspected after join. */
static unsigned writes; /* TX worker-only physical call count. */
static int uncertain; /* Immutable injected output policy. */


/* EN/RU: Explicit fatal fixture invariant. / Явный fatal invariant. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "terminal fault: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Fake only the native read RPC, returning coalesced fixture frames then explicit EOF.
 * Подменить только native read RPC; coalesced frames и затем EOF. */
void __wrap_styxclientreadwatch(StyxFid *fid, unsigned long long offset, void *bytes, size_t size,
    StyxCancel *cancel, StyxTransfer *result)
{
    (void)fid;
    (void)offset;
    (void)cancel;
    memset(result, 0, sizeof(*result));
    result->success = result->confirmed = 1;
    result->count = input_size-input_at < size ? input_size-input_at : size;
    memcpy(bytes, input+input_at, result->count);
    input_at += result->count;
}


/* EN/RU: Fake exact two-byte output prefixes or one uncertain attempt; no actual transport.
 * Подменить two-byte prefix либо один uncertain attempt; это не actual transport. */
void __wrap_styxclientwritecancel(StyxFid *fid, unsigned long long offset, const void *bytes, size_t size,
    StyxCancel *cancel, StyxTransfer *result)
{
    (void)fid;
    (void)offset;
    (void)cancel;
    memset(result, 0, sizeof(*result));
    result->confirmed = 1;
    result->success = !uncertain;
    result->uncertain = uncertain;
    result->count = size < 2 ? size : 2;
    check(output_size+result->count <= sizeof(output), "output bounds");
    memcpy(output+output_size, bytes, result->count);
    output_size += result->count;
    writes++;
}


/* EN/RU: Private test construction isolates workers from endpoint setup (covered by real PTY cases).
 * Private fixture construction изолирует worker tests от setup, проверенного настоящим PTY. */
static StyxBidirControl *fixture(unsigned limit, unsigned slots)
{
    StyxBidirControl *control = calloc(1, sizeof(*control));
    check(control && !pthread_mutex_init(&control->lock, NULL), "native fixture mutex");
    pthread_condattr_t attr;
    check(!pthread_condattr_init(&attr) && !pthread_condattr_setclock(&attr, CLOCK_MONOTONIC) &&
        !pthread_cond_init(&control->changed, &attr), "native fixture condition");
    pthread_condattr_destroy(&attr);
    control->limit = limit;
    control->slots = slots;
    control->extent = limit+32;
    control->incoming.bytes = malloc(control->extent*slots);
    control->outgoing.bytes = malloc(control->extent*slots);
    control->decode = malloc(control->extent);
    check(control->incoming.bytes && control->outgoing.bytes && control->decode, "bounded fixture storage");
    return control;
}


/* EN/RU: Run one receiver fault/coalescing variant; CRC/quota/truncation must publish no record.
 * Проверить receiver faults/coalescing; CRC/quota/truncation не публикуют record. */
static void receiving(const char *mode)
{
    size_t size;
    unsigned char payload[16] = {0, 128, 255};
    check(fvm_srecord_write(7, payload, !strcmp(mode, "limit") ? 16 : 3,
        input, sizeof(input), &input_size) == FVM_S_OK, "input fixture encoding");
    if (!strcmp(mode, "crc"))
    {
        input[input_size-1] ^= 1;
    }
    else if (!strcmp(mode, "truncated"))
    {
        input_size--;
    }
    else if (!strcmp(mode, "batch"))
    {
        check(fvm_srecord_write(8, NULL, 0, input+input_size, sizeof(input)-input_size, &size) == FVM_S_OK,
            "second coalesced fixture record");
        input_size += size;
    }
    StyxBidirControl *control = fixture(!strcmp(mode, "limit") ? 4 : 64, 1);
    check(!pthread_create(&control->workers[0], NULL, controlreceive, control), "native receiver fixture");
    control->started = 1;
    if (!strcmp(mode, "batch"))
    {
        for (unsigned i = 0; i < 2; i++)
        {
            unsigned long tag;
            size_t count;
            unsigned char bytes[16];
            check(styxbidircontrolwait(control, 2000) &&
                styxbidircontrolnext(control, &tag, bytes, sizeof(bytes), &count) == STYX_CONTROL_RECORD &&
                tag == 7+i && count == (i ? 0U : 3U), "one record per call with one-slot backpressure");
        }
    }
    check(!styxbidircontrolwait(control, 2000), "terminal receive wait");
    check(styxbidircontrolstate(control) == (!strcmp(mode, "batch") ? STYX_CONTROL_EOF : STYX_CONTROL_PROTOCOL),
        "sticky decoder termination");
    unsigned long tag = 77;
    size_t count = 99;
    check(styxbidircontrolnext(control, &tag, NULL, 0, &count) == STYX_CONTROL_END && tag == 77 && count == 99,
        "failed decode never exposes partial record");
    styxbidircontrolfree(control);
}


/* EN/RU: Verify exact output FIFO and sticky uncertain refusal; record acceptance differs from delivery.
 * Проверить exact output FIFO и sticky uncertain refusal; acceptance не равно delivery. */
static void sending(const char *mode)
{
    StyxBidirControl *control = fixture(64, 2);
    unsigned char payload[] = {0, 128, 255};
    check(styxbidircontrolsend(control, 7, payload, sizeof(payload)) &&
        styxbidircontrolsend(control, 8, NULL, 0) && !styxbidircontrolsend(control, 9, NULL, 0),
        "deterministic TX queue bound before worker start");
    check(!styxbidircontrolflush(control, 0), "zero-budget flush preserves queued frames");
    uncertain = !strcmp(mode, "uncertain");
    check(!pthread_create(&control->workers[0], NULL, controltransmit, control), "native writer fixture");
    control->started = 1;
    check(styxbidircontrolflush(control, 2000) == !uncertain, "exact native drain outcome");
    if (uncertain)
    {
        check(styxbidircontrolstate(control) == STYX_CONTROL_WRITE_FAILED &&
            !styxbidircontrolsend(control, 9, NULL, 0), "uncertain output poisons future enqueue");
    }
    styxbidircontrolfree(control);
    if (uncertain)
    {
        check(writes == 1 && output_size == 2, "unknown tail never replayed");
    }
    else
    {
        FVM_SRecord first, second;
        size_t prefix, suffix;
        check(writes > 2 && fvm_srecord_read(output, output_size, 64, &first, &prefix) == FVM_S_OK &&
            first.tag == 7 && first.size == 3 && !memcmp(first.payload, payload, 3) &&
            fvm_srecord_read(output+prefix, output_size-prefix, 64, &second, &suffix) == FVM_S_OK &&
            second.tag == 8 && second.size == 0 && prefix+suffix == output_size, "short writes never interleave records");
    }
}


/* EN/RU: Dispatch a fixture variant; production path is covered separately over real original 9P.
 * Запустить fixture variant; production path отдельно проверен через настоящий original 9P. */
int main(int argc, char **argv)
{
    check(argc == 2, "fault variant");
    if (!strcmp(argv[1], "short") || !strcmp(argv[1], "uncertain"))
    {
        sending(argv[1]);
    }
    else
    {
        receiving(argv[1]);
    }
    puts("PASS terminal worker fault and bounded record ownership");
    return 0;
}
