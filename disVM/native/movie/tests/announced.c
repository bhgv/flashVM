/* Copyright (c) 2026 bhgv
 * EN: Actual player auto-binds announced roots on the existing 9P connection and refuses stale data.
 * RU: Настоящий player auto-binds announced roots на existing 9P connection и отклоняет stale data.
 */
#include "receiver.h"
#include "announcement.h"
#include "player.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct TestSession
{
    StyxClient *client; /* Borrowed original trusted connection. */
    StyxFid *control; /* Owned duplex control fid, distinct from resource attaches. */
    unsigned long long offset; /* Shared stream offset across reads/writes. */
    StyxMovieSession *session; /* Owned automatic receiver/client pin. */
    unsigned char first[512]; /* Captured initial declaration/offer for replay checks. */
    size_t size; /* Captured byte count. */
    int change; /* Inject scope replacement from first staged resource chunk. */
    FVM_DesktopWrite write; /* Borrowed real player sink through synchronous fetch. */
    void *sink; /* Borrowed real player collector. */
} TestSession;


/* EN/RU: Explicit fixture contract remains checked in optimized builds.
 * Явная проверка fixture contract сохраняется при optimization. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "announcement: %s\n", why);
        exit(9);
    }
}


/* EN/RU: Pump actual three-byte Tread fragments until new state; no guest execution or polling loop.
 * Выполнить реальные Tread fragments до нового state, без guest execution/polling. */
static void pump(TestSession *test, unsigned long long after, int capture)
{
    for (unsigned i = 0; i < 512; i++)
    {
        unsigned long long generation;
        int state = styxmovieavailable(test->session, &generation);
        if (state < 0 || (state == 1 && generation > after))
        {
            return;
        }
        unsigned char bytes[3];
        StyxTransfer result;
        styxclientread(test->control, test->offset, bytes, sizeof(bytes), &result);
        check(result.success && result.confirmed && !result.uncertain && result.count &&
            result.count <= sizeof(bytes), "control Tread");
        test->offset += result.count;
        if (capture)
        {
            check(result.count <= sizeof(test->first)-test->size, "saved offer capacity");
            memcpy(test->first+test->size, bytes, result.count);
            test->size += result.count;
        }
        check(styxmoviecontrol(test->session, bytes, result.count), "control record acceptance");
    }
    check(0, "bounded control transfer");
}


/* EN/RU: Fixture host command runs through actual Twrite on the already-open control fid.
 * Fixture host command выполняется через настоящий Twrite control fid. */
static void command(TestSession *test, const char *name)
{
    StyxTransfer result;
    styxclientwrite(test->control, test->offset, name, strlen(name), &result);
    check(result.success && result.confirmed && !result.uncertain && result.count == strlen(name), "control Twrite");
    test->offset += result.count;
}


/* EN/RU: Stage real bytes, then replace the actual server scope while that resource remains open.
 * Подготовить реальные bytes, затем заменить server scope при ещё открытом ресурсе. */
static FVM_Result changing(const void *bytes, size_t size, void *state)
{
    TestSession *test = state;
    FVM_Result result = test->write(bytes, size, test->sink);
    if (result == FVM_OK && test->change)
    {
        test->change = 0;
        command(test, "replace");
        pump(test, 1, 0);
    }
    return result;
}


/* EN/RU: Native wrapper never reenters VM/SDL; only the genuine bridge handles resource acquisition.
 * Native wrapper не входит в VM/SDL; ресурсы получает настоящий bridge. */
static FVM_Result changefetch(unsigned purpose, const char *reference, const char *base,
    FVM_DesktopWrite write, void *state, char *resolved, size_t capacity, void *userdata)
{
    TestSession *test = userdata;
    test->write = write;
    test->sink = state;
    return styxmovieannouncedfetch(purpose, reference, base, changing, test, resolved, capacity, test->session);
}


/* EN/RU: Run the actual managed player with one selected receiver; require exact expected outcome.
 * Запустить настоящий managed player с выбранным receiver, требовать точный результат. */
static void run(const FVM_DesktopOptions *options, const FVM_DesktopSource *source,
    TestSession *test, int expected, int changing_scope)
{
    FVM_DesktopManagedLoader loader;
    const char *error = NULL;
    fvm_desktop_managed_loader_init(&loader);
    loader.max_loads = 6;
    loader.fetch = changing_scope ? changefetch : styxmovieannouncedfetch;
    loader.userdata = changing_scope ? (void *)test : (void *)test->session;
    int result = fvm_desktop_run_source_managed(options, source, &loader, &error);
    check(result == expected && (result || !error), "actual advertised player result");
}


/* EN/RU: Feed a valid but incompatible schema; decoding must fail closed before any resource read.
 * Передать валидную несовместимую schema; отказ до любого resource read. */
static void wrongversion(StyxMovieSession *session)
{
    FVM_SBinding binding = {0};
    unsigned char payload[128], bytes[256];
    size_t size, frame;
    binding.tag = 7;
    binding.version = 2;
    binding.type_size = sizeof(MOVIE_OFFER_TYPE)-1;
    memcpy(binding.type, MOVIE_OFFER_TYPE, sizeof(MOVIE_OFFER_TYPE));
    check(fvm_sbindings_payload_write(&binding, 1, payload, sizeof(payload), &size) == FVM_S_OK &&
        fvm_srecord_write(0, payload, size, bytes, sizeof(bytes), &frame) == FVM_S_OK &&
        styxmoviecontrol(session, bytes, frame), "future binding declaration");
    check(movie_offer_record(7, MOVIE_OFFER_READY, 1, bytes, sizeof(bytes), &size) &&
        !styxmoviecontrol(session, bytes, size), "future schema accepted");
}


/* EN/RU: Exercise initial pending, automatic Twalk, update/replay, retirement and link EOF.
 * Проверить pending, automatic Twalk, update/replay, retirement и link EOF. */
int movie_test_announced(StyxClient *client, const FVM_DesktopOptions *options,
    const FVM_DesktopSource *source, const char *mode)
{
    TestSession test = {0};
    test.client = client;
    test.session = styxmoviesession(client);
    check(test.session && styxmovieavailable(test.session, NULL) == 0, "new receiver pending");
    run(options, source, &test, FVM_DESKTOP_EXIT_PLAYBACK, 0);
    test.control = styxclientwalk(client, "ctl");
    check(test.control != NULL, "control Twalk");
    check(styxclientopen(test.control, 2), "control Topen");
    pump(&test, 0, 1);
    if (!strcmp(mode, "crc") || !strcmp(mode, "version"))
    {
        StyxMovieSession *bad = styxmoviesession(client);
        check(bad != NULL, "private malformed receiver");
        if (!strcmp(mode, "crc"))
        {
            test.first[test.size-1] ^= 1;
            check(!styxmoviecontrol(bad, test.first, test.size), "bad CRC accepted");
        }
        else
        {
            wrongversion(bad);
        }
        styxmoviesessionfree(test.session);
        test.session = bad;
        check(styxmovieavailable(test.session, NULL) == -1, "malformed receiver not retired");
        run(options, source, &test, FVM_DESKTOP_EXIT_PLAYBACK, 0);
    }
    else
    {
        if (!strcmp(mode, "changing"))
        {
            test.change = 1;
            run(options, source, &test, FVM_DESKTOP_EXIT_PLAYBACK, 1);
            run(options, source, &test, FVM_DESKTOP_EXIT_OK, 0);
        }
        else
        {
            run(options, source, &test, FVM_DESKTOP_EXIT_OK, 0);
        }
        if (!strcmp(mode, "disconnect"))
        {
            styxclientshutdown(client);
        }
        else if (!strcmp(mode, "eof"))
        {
            styxmoviecontrolend(test.session);
            check(styxclientalive(client), "control EOF closed borrowed connection");
        }
        else
        {
            if (strcmp(mode, "changing"))
            {
                command(&test, "replace");
                pump(&test, 1, 0);
            }
            check(styxmoviecontrol(test.session, test.first, test.size), "stale valid offer rejected as corrupt");
            unsigned long long generation;
            check(styxmovieavailable(test.session, &generation) == 1 && generation == 2, "offer generation rolled back");
            run(options, source, &test, FVM_DESKTOP_EXIT_OK, 0);
            StyxFid *stale = styxclientwalk(client, "movie/1");
            check(!stale, "stale name admitted after scope change");
            command(&test, "close");
            pump(&test, 2, 0);
        }
        check(styxmovieavailable(test.session, NULL) == -1, "closed stream/link still usable");
        run(options, source, &test, FVM_DESKTOP_EXIT_PLAYBACK, 0);
    }
    styxclientclunk(test.control);
    styxmoviesessionfree(test.session);
    puts("PASS automatically announced movie endpoint on original Styx connection");
    return 0;
}
