/* Leased frame contract with allocation/identity exhaustion injection; no VM/SDL startup. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
static size_t allocation, fail_at, live;
static void *allocate(size_t n, size_t size)
{
    void *p;
    ++allocation;
    if (allocation == fail_at)
        return 0;
    p = calloc(n, size);
    if (p)
        ++live;
    return p;
}
static void release(void *p)
{
    if (p)
        --live;
    free(p);
}
#define calloc allocate
#define free release
#include "../common/resource_frame.c"
#undef calloc
#undef free
#define CHECK(x)                                                                                   \
    do                                                                                             \
    {                                                                                              \
        if (!(x))                                                                                  \
        {                                                                                          \
            fprintf(stderr, "frame-resource:%d %s\n", __LINE__, #x);                               \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)
static unsigned long next_id;
static int call(FVM_RDispatch *d, FVM_RHandle target, unsigned long session,
                unsigned long operation, const unsigned long *fields, size_t count,
                FVM_RReply *reply)
{
    FVM_RRequest q;
    unsigned char data[64];
    size_t n = 0, i, done;
    for (i = 0; i < count; ++i)
        n += frame_put(data + n, fields[i]);
    memset(&q, 0, sizeof(q));
    q.session = session;
    q.id = ++next_id;
    q.target = target;
    q.operation = operation;
    q.data = data;
    q.size = n;
    return fvm_rdispatch_post(d, &q) == FVM_R_OK && fvm_rdispatch_pump(d, 1, &done) == FVM_R_OK &&
           done == 1 && fvm_rdispatch_take(d, session, q.id, reply) == FVM_R_OK;
}
int main(void)
{
    FVM_RDispatch *d = 0;
    FVM_DesktopResourceFrame *a = 0, *saved;
    FVM_RHandle target, before_target;
    FVM_DesktopFrame f, bad;
    FVM_RReply reply;
    FVM_REvent event;
    FVM_RWatch watch;
    FVM_DesktopResourceFrameInfo info, before, first, second;
    FVM_DesktopFrameLease leases[FVM_DESKTOP_RFRAME_LEASES];
    unsigned char pixels[24], expected[16], encoded[FVM_R_BYTES];
    unsigned long args[6];
    size_t i, n, count;
    FVM_RRequest q;
    FVM_SControlLimits ceiling = {19, 4064, 4096, 4000};
    FVM_SControlLimits wanted = {19, 992, 1024, 768};
    FVM_SControlBoundary boundary = {7, 8, 1};
    FVM_SControlExchange sender, receiver;
    FVM_SControlMessage offer, ack;
    FVM_SMediaPiece piece;
    unsigned char exported[FVM_SSLOT_BYTES], preserved[FVM_SSLOT_BYTES];
    size_t exported_size, allocations_before;
    char source[FVM_DESKTOP_RFRAME_SOURCE + 2];
    CHECK(fvm_rdispatch_create(0, &d) == FVM_R_OK);
    memset(&target, 0xa5, sizeof(target));
    before_target = target;
    for (i = 1; i <= FVM_DESKTOP_RFRAME_SLOTS + 1; ++i)
    {
        fail_at = i;
        allocation = 0;
        saved = a;
        CHECK(fvm_desktop_resource_frame_create(d, "/display/frame", 16, &a, &target) ==
                  FVM_R_NOMEM &&
              a == saved && !memcmp(&target, &before_target, sizeof(target)));
        CHECK(!live && fvm_rdispatch_resources(d, 1, 0, 0, &count) == FVM_R_OK && !count);
    }
    fail_at = 0;
    allocation = 0;
    CHECK(fvm_desktop_resource_frame_create(d, "/display/frame", 16, &a, &target) == FVM_R_OK &&
          allocation == FVM_DESKTOP_RFRAME_SLOTS + 1);
    CHECK(fvm_rdispatch_grant(d, target, 1, FVM_R_READ | FVM_R_SUBSCRIBE) == FVM_R_OK &&
          fvm_rdispatch_grant(d, target, 2, FVM_R_READ) == FVM_R_OK);
    CHECK(fvm_rdispatch_subscribe(d, 1, target, &watch) == FVM_R_OK);
    CHECK(call(d, target, 1, FVM_DESKTOP_RFRAME_INFO, 0, 0, &reply) && reply.result == FVM_R_BUSY);
    memset(&f, 0, sizeof(f));
    f.struct_size = sizeof(f);
    f.format = FVM_DESKTOP_FRAME_RGBA8;
    f.reason = FVM_DESKTOP_FRAME_REFRESH;
    f.width = 2;
    f.height = 2;
    f.stride = 12;
    f.byte_count = sizeof(pixels);
    f.pixels = pixels;
    f.source = "memory://frame";
    f.presentations = f.total_presentations = 1;
    for (i = 0; i < sizeof(pixels); ++i)
        pixels[i] = (unsigned char)i;
    memcpy(expected, pixels, 8);
    memcpy(expected + 8, pixels + 12, 8);
    CHECK(fvm_desktop_resource_frame_capture(a, &f) == FVM_R_OK &&
          allocation == FVM_DESKTOP_RFRAME_SLOTS + 1);
    bad = f;
    bad.width = 3;
    CHECK(fvm_desktop_resource_frame_capture(a, &bad) == FVM_R_LIMIT && a->sequence == 1);
    saved = a;
    CHECK(fvm_desktop_resource_frame_create(d, "/display/frame", 16, &a, &target) ==
              FVM_R_CONFLICT &&
          a == saved && live == FVM_DESKTOP_RFRAME_SLOTS + 1);
    memset(pixels, 0xbb, sizeof(pixels));
    CHECK(fvm_rdispatch_event_take(d, 1, watch, &event) == FVM_R_OK &&
          event.kind == FVM_DESKTOP_RFRAME_PRESENTED);
    CHECK(fvm_desktop_resource_frame_read(event.data, event.size, &info) == FVM_R_OK &&
          !info.lease.id && info.sequence == 1 && info.row_bytes == 8 && info.byte_count == 16);
    before = info;
    for (i = 0; i < event.size; ++i)
        CHECK(fvm_desktop_resource_frame_read(event.data, i, &info) == FVM_R_INVALID &&
              !memcmp(&info, &before, sizeof(info)));
    memcpy(encoded, event.data, event.size);
    encoded[0] = 0x82;
    CHECK(fvm_desktop_resource_frame_read(encoded, event.size, &info) == FVM_R_INVALID);
    CHECK(call(d, target, 1, FVM_DESKTOP_RFRAME_ACQUIRE, 0, 0, &reply) &&
          reply.result == FVM_R_OK &&
          fvm_desktop_resource_frame_read(reply.data, reply.size, &first) == FVM_R_OK &&
          first.lease.id);
    CHECK(fvm_scontrol_init(&sender, &ceiling) == FVM_S_OK &&
          fvm_scontrol_init(&receiver, &ceiling) == FVM_S_OK);
    memset(exported, 0xa5, sizeof(exported));
    memcpy(preserved, exported, sizeof(exported));
    exported_size = 99;
    allocations_before = allocation;
    CHECK(fvm_desktop_resource_frame_write_part(a, 1, first.lease, &sender, FVM_SMEDIA_FILE, 40, 0,
                                                exported, sizeof(exported),
                                                &exported_size) == FVM_R_BUSY);
    CHECK(exported_size == 99 && !memcmp(exported, preserved, sizeof(exported)));
    CHECK(fvm_scontrol_propose_at(&sender, 1, &wanted, &boundary, &offer) == FVM_S_OK &&
          fvm_scontrol_respond(&receiver, &offer, &ack) == FVM_S_OK &&
          fvm_scontrol_commit(&receiver, &ack) == FVM_S_OK &&
          fvm_scontrol_commit(&sender, &ack) == FVM_S_OK);
    CHECK(fvm_desktop_resource_frame_write_part(a, 2, first.lease, &sender, FVM_SMEDIA_FILE, 40, 0,
                                                exported, sizeof(exported),
                                                &exported_size) == FVM_R_DENIED);
    CHECK(fvm_desktop_resource_frame_write_part(a, 1, first.lease, &sender, FVM_SMEDIA_FILE, 40, 0,
                                                exported, sizeof(exported) - 1,
                                                &exported_size) == FVM_R_LIMIT);
    CHECK(fvm_desktop_resource_frame_write_part(a, 1, first.lease, &sender, FVM_SMEDIA_FILE, 40, 1,
                                                exported, sizeof(exported),
                                                &exported_size) == FVM_R_INVALID);
    CHECK(exported_size == 99 && !memcmp(exported, preserved, sizeof(exported)));
    CHECK(fvm_desktop_resource_frame_write_part(a, 1, first.lease, &sender, FVM_SMEDIA_FILE, 40, 0,
                                                exported, sizeof(exported),
                                                &exported_size) == FVM_R_OK);
    CHECK(exported_size == sizeof(exported) && allocation == allocations_before &&
          fvm_smedia_read(exported, exported_size, 19, &piece) == FVM_S_OK &&
          piece.info.stream == 7 && piece.info.epoch == 8 && piece.info.object == first.sequence &&
          piece.info.total == 16 && piece.info.mode == FVM_SMEDIA_FILE &&
          piece.info.time_ms == 40 && piece.size == 16 && !memcmp(piece.data, expected, 16));
    a->frames[a->leases[first.lease.id - 1].slot].info.byte_count = FVM_SMEDIA_MAX_BYTES + 1;
    CHECK(fvm_desktop_resource_frame_write_part(a, 1, first.lease, &receiver, FVM_SMEDIA_FILE, 40,
                                                0, exported, sizeof(exported),
                                                &exported_size) == FVM_R_LIMIT);
    a->frames[a->leases[first.lease.id - 1].slot].info.byte_count = 16;
    boundary.object = 2;
    CHECK(fvm_scontrol_propose_at(&sender, 2, &wanted, &boundary, &offer) == FVM_S_OK &&
          fvm_scontrol_respond(&receiver, &offer, &ack) == FVM_S_OK &&
          fvm_scontrol_commit(&sender, &ack) == FVM_S_OK);
    CHECK(fvm_desktop_resource_frame_write_part(a, 1, first.lease, &sender, FVM_SMEDIA_FILE, 40, 0,
                                                exported, sizeof(exported),
                                                &exported_size) == FVM_R_CONFLICT);
    CHECK(fvm_desktop_resource_frame_capture(a, &f) == FVM_R_OK);
    CHECK(call(d, target, 2, FVM_DESKTOP_RFRAME_ACQUIRE, 0, 0, &reply) &&
          reply.result == FVM_R_OK &&
          fvm_desktop_resource_frame_read(reply.data, reply.size, &second) == FVM_R_OK);
    memset(pixels, 0xcc, sizeof(pixels));
    CHECK(fvm_desktop_resource_frame_capture(a, &f) == FVM_R_OK && a->sequence == 3);
    CHECK(fvm_desktop_resource_frame_capture(a, &f) == FVM_R_BUSY && a->dropped == 1);
    CHECK(fvm_desktop_resource_frame_notify(&f, a) == FVM_OK && a->dropped == 2 &&
          a->sequence == 3);
    bad = f;
    bad.pixels = 0;
    CHECK(fvm_desktop_resource_frame_capture(a, &bad) == FVM_R_INVALID && a->dropped == 2);
    args[0] = first.lease.id;
    args[1] = first.lease.generation;
    args[2] = 0;
    args[3] = 512;
    CHECK(call(d, target, 1, FVM_DESKTOP_RFRAME_CHUNK, args, 4, &reply) &&
          reply.result == FVM_R_OK && reply.size == 16 && !memcmp(reply.data, expected, 16));
    CHECK(call(d, target, 2, FVM_DESKTOP_RFRAME_CHUNK, args, 4, &reply) &&
          reply.result == FVM_R_DENIED);
    args[2] = 1;
    args[3] = 0;
    args[4] = 1;
    args[5] = 2;
    CHECK(call(d, target, 1, FVM_DESKTOP_RFRAME_REGION, args, 6, &reply) &&
          reply.result == FVM_R_OK && reply.size == 8 && !memcmp(reply.data, expected + 4, 4) &&
          !memcmp(reply.data + 4, expected + 12, 4));
    CHECK(call(d, target, 2, FVM_DESKTOP_RFRAME_REGION, args, 6, &reply) &&
          reply.result == FVM_R_DENIED && !reply.size);
    CHECK(call(d, target, 1, FVM_DESKTOP_RFRAME_REGION, args, 5, &reply) &&
          reply.result == FVM_R_INVALID && !reply.size);
    args[4] = FVM_SRECORD_U32;
    CHECK(call(d, target, 1, FVM_DESKTOP_RFRAME_REGION, args, 6, &reply) &&
          reply.result == FVM_R_INVALID && !reply.size);
    args[4] = 0;
    CHECK(call(d, target, 1, FVM_DESKTOP_RFRAME_REGION, args, 6, &reply) &&
          reply.result == FVM_R_INVALID && !reply.size);
    args[3] = 512;
    args[2] = 16;
    CHECK(call(d, target, 1, FVM_DESKTOP_RFRAME_CHUNK, args, 4, &reply) &&
          reply.result == FVM_R_OK && !reply.size);
    args[2] = 17;
    CHECK(call(d, target, 1, FVM_DESKTOP_RFRAME_CHUNK, args, 4, &reply) &&
          reply.result == FVM_R_INVALID);
    args[2] = 0;
    args[3] = 513;
    CHECK(call(d, target, 1, FVM_DESKTOP_RFRAME_CHUNK, args, 4, &reply) &&
          reply.result == FVM_R_INVALID);
    CHECK(call(d, target, 2, FVM_DESKTOP_RFRAME_RELEASE, args, 2, &reply) &&
          reply.result == FVM_R_DENIED);
    CHECK(call(d, target, 1, FVM_DESKTOP_RFRAME_RELEASE, args, 2, &reply) &&
          reply.result == FVM_R_OK);
    CHECK(call(d, target, 1, FVM_DESKTOP_RFRAME_CHUNK, args, 4, &reply) &&
          reply.result == FVM_R_UNKNOWN);
    CHECK(fvm_desktop_resource_frame_write_part(a, 1, first.lease, &sender, FVM_SMEDIA_FILE, 40, 0,
                                                exported, sizeof(exported),
                                                &exported_size) == FVM_R_UNKNOWN);
    CHECK(fvm_desktop_resource_frame_capture(a, &f) == FVM_R_OK && a->sequence == 4);
    args[0] = second.lease.id;
    args[1] = second.lease.generation;
    args[2] = 8;
    args[3] = 7;
    CHECK(call(d, target, 2, FVM_DESKTOP_RFRAME_CHUNK, args, 4, &reply) &&
          reply.result == FVM_R_OK && reply.size == 7 && reply.data[0] == 0xbb);
    CHECK(fvm_desktop_resource_frame_revoke(a, 2) == FVM_R_OK);
    CHECK(fvm_rdispatch_grant(d, target, 2, FVM_R_READ) == FVM_R_OK);
    CHECK(call(d, target, 2, FVM_DESKTOP_RFRAME_CHUNK, args, 4, &reply) &&
          reply.result == FVM_R_UNKNOWN);
    for (i = 0; i < FVM_DESKTOP_RFRAME_LEASES; ++i)
    {
        CHECK(call(d, target, 1, FVM_DESKTOP_RFRAME_ACQUIRE, 0, 0, &reply) &&
              reply.result == FVM_R_OK &&
              fvm_desktop_resource_frame_read(reply.data, reply.size, &info) == FVM_R_OK);
        leases[i] = info.lease;
    }
    CHECK(call(d, target, 1, FVM_DESKTOP_RFRAME_ACQUIRE, 0, 0, &reply) &&
          reply.result == FVM_R_LIMIT);
    CHECK(fvm_desktop_resource_frame_revoke(a, 1) == FVM_R_OK &&
          fvm_rdispatch_grant(d, target, 1, FVM_R_READ) == FVM_R_OK);
    for (i = 0; i < FVM_DESKTOP_RFRAME_LEASES; ++i)
    {
        args[0] = leases[i].id;
        args[1] = leases[i].generation;
        CHECK(call(d, target, 1, FVM_DESKTOP_RFRAME_RELEASE, args, 2, &reply) &&
              reply.result == FVM_R_UNKNOWN);
    }
    a->leases[0].generation = FVM_SRECORD_U32;
    CHECK(call(d, target, 1, FVM_DESKTOP_RFRAME_ACQUIRE, 0, 0, &reply) &&
          reply.result == FVM_R_OK &&
          fvm_desktop_resource_frame_read(reply.data, reply.size, &info) == FVM_R_OK &&
          info.lease.id == 1);
    args[0] = info.lease.id;
    args[1] = info.lease.generation;
    CHECK(call(d, target, 1, FVM_DESKTOP_RFRAME_RELEASE, args, 2, &reply) &&
          reply.result == FVM_R_OK && !a->leases[0].generation);
    memset(source, 'x', sizeof(source) - 1);
    source[sizeof(source) - 1] = 0;
    f.source = source;
    if (ULONG_MAX > FVM_SRECORD_U32)
        f.movie_ticks = FVM_SRECORD_U32 + 1UL;
    CHECK(fvm_desktop_resource_frame_capture(a, &f) == FVM_R_OK);
    CHECK(call(d, target, 1, FVM_DESKTOP_RFRAME_INFO, 0, 0, &reply) && reply.result == FVM_R_OK &&
          fvm_desktop_resource_frame_read(reply.data, reply.size, &info) == FVM_R_OK &&
          (info.flags & FVM_DESKTOP_RFRAME_SOURCE_TRUNCATED) && info.dropped == 2);
    if (ULONG_MAX > FVM_SRECORD_U32)
        CHECK((info.flags & FVM_DESKTOP_RFRAME_COUNTERS_TRUNCATED) &&
              info.movie_ticks == FVM_SRECORD_U32);
    n = frame_encode(&info, encoded);
    encoded[n] = 0x80;
    CHECK(fvm_desktop_resource_frame_read(encoded, n + 1, &before) == FVM_R_INVALID);
    CHECK(call(d, target, 1, FVM_DESKTOP_RFRAME_ACQUIRE, 0, 0, &reply) && reply.result == FVM_R_OK);
    a->sequence = FVM_SRECORD_U32;
    CHECK(fvm_desktop_resource_frame_capture(a, &f) == FVM_R_LIMIT);
    memset(&q, 0, sizeof(q));
    q.session = 1;
    q.id = ++next_id;
    q.target = target;
    q.operation = FVM_DESKTOP_RFRAME_INFO;
    CHECK(fvm_rdispatch_post(d, &q) == FVM_R_OK &&
          fvm_desktop_resource_frame_destroy(&a) == FVM_R_OK && !a);
    CHECK(fvm_rdispatch_pump(d, 1, &count) == FVM_R_OK &&
          fvm_rdispatch_take(d, 1, q.id, &reply) == FVM_R_OK && reply.result == FVM_R_UNKNOWN &&
          fvm_rdispatch_destroy(&d) == FVM_R_OK && !live);
    puts("PASS leased copied frames, bounded chunks, backpressure, revoke, failure and identity "
         "exhaustion");
    return 0;
}
