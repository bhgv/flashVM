/* Copyright (c) 2026 bhgv
 * EN: Actual FlashVM runs while an independent native worker consumes control announcements.
 * RU: Настоящий FlashVM работает при independent native worker, принимающем control announcements.
 */
#include "monitor.h"
#include "protocol.h"
#include "player.h"
#include <poll.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Arm owner-local constructor failure, without affecting other native thread creation.
 * Включить owner-local constructor failure без воздействия на другие native threads. */
extern void movie_test_fail_thread_once(void);


/* EN/RU: Explicit always-active native fixture invariant; diagnostic borrowed through return.
 * Явный native fixture invariant во всех builds; diagnostic borrowed. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "movie monitor: %s\n", why);
        exit(12);
    }
}


/* EN/RU: Native context outlives worker and release marker; main inspects captured bytes only after join.
 * Native context живёт дольше worker/release marker; main читает captured bytes после join. */
typedef struct Test
{
    StyxClient *client; /* Borrowed through actual player/native test. */
    StyxMovieSession *session; /* Owned by main or borrowed under monitor pin. */
    StyxMovieMonitor *monitor; /* Owned automatic control reader. */
    unsigned char bytes[2048]; /* Bounded generic dispatcher storage, no VM pointers. */
    size_t size; /* Worker-only extent until join. */
    unsigned released; /* Release callback count, main-only after join. */
    int refuse; /* Immutable native-dispatch policy refusal. */
    int change; /* Main/player-only source change injection. */
    FVM_DesktopWrite write; /* Borrowed actual player sink through one synchronous fetch. */
    void *sink; /* Borrowed native collector, never used on the monitor worker. */
} Test;


/* EN/RU: Native dispatcher copies raw fragments for later main-thread processing;1/0, no VM/SDL/wait.
 * Native dispatcher копирует raw fragments для main;1/0, без VM/SDL/wait. */
static int received(void *state, const void *bytes, size_t size)
{
    Test *test = state;
    if (test->refuse || size > sizeof(test->bytes)-test->size)
    {
        return 0;
    }
    memcpy(test->bytes+test->size, bytes, size);
    test->size += size;
    return 1;
}


/* EN/RU: Count exact final context release after worker callbacks retire; stack storage remains main-owned.
 * Считать final context release после callbacks; stack storage остаётся у main. */
static void released(void *state)
{
    ((Test *)state)->released++;
}


/* EN/RU: Await native readiness until requested generation or terminal state; no polling guest code.
 * Дождаться native readiness до generation/terminal state, без guest polling. */
static StyxMovieMonitorState await(Test *test, unsigned long long generation, int terminal)
{
    for (unsigned attempt = 0; attempt < 32; attempt++)
    {
        StyxMovieMonitorState state;
        check(styxmoviecontrolstate(test->monitor, &state), "monitor snapshot");
        if ((terminal && state.reason != STYX_MOVIE_READING) ||
            (!terminal && state.available == 1 && state.generation >= generation))
        {
            return state;
        }
        check(state.reason == STYX_MOVIE_READING, "premature reader retirement");
        struct pollfd ready = {styxmoviecontrolfd(test->monitor), POLLIN, 0};
        check(poll(&ready, 1, 5000) == 1, "monitor readiness deadline");
    }
    check(0, "readiness transition bound");
    StyxMovieMonitorState empty = {0};
    return empty;
}


/* EN/RU: Change fixture source through independent control fid; automatic reader owns its own read-only fid.
 * Изменить fixture source через отдельный fid; automatic reader владеет своим read-only fid. */
static void command(Test *test, const char *text)
{
    StyxFid *fid = styxclientwalk(test->client, "ctl");
    check(fid && styxclientopen(fid, 2), "separate command fid");
    StyxTransfer out;
    styxclientwrite(fid, 0, text, strlen(text), &out);
    check(out.success && out.confirmed && out.count == strlen(text) && !out.uncertain, "source change write");
    styxclientclunk(fid);
}


/* EN/RU: Actual player sink receives bytes on its owner; source update must arrive through native monitor.
 * Actual player sink работает на owner; source update приходит только через native monitor. */
static FVM_Result changing(const void *bytes, size_t size, void *state)
{
    Test *test = state;
    FVM_Result result = test->write(bytes, size, test->sink);
    if (result == FVM_OK && size && test->change)
    {
        test->change = 0;
        command(test, "replace");
        (void)await(test, 2, 0);
    }
    return result;
}


/* EN/RU: Borrow real fetch inputs and collector only through this synchronous call; never on I/O worker.
 * Borrow actual fetch inputs/collector только на synchronous call, не на I/O worker. */
static FVM_Result fetch(unsigned purpose, const char *reference, const char *base,
    FVM_DesktopWrite write, void *state, char *resolved, size_t capacity, void *userdata)
{
    Test *test = userdata;
    test->write = write;
    test->sink = state;
    return styxmovieannouncedfetch(purpose, reference, base, changing, test, resolved, capacity, test->session);
}


/* EN/RU: Execute actual managed player on its original thread while native monitor continues control reads.
 * Выполнить managed player на исходном thread при продолжающемся native control reader. */
static void run(Test *test, const FVM_DesktopOptions *options, const FVM_DesktopSource *source, int expected)
{
    FVM_DesktopManagedLoader loader;
    fvm_desktop_managed_loader_init(&loader);
    loader.fetch = fetch;
    loader.userdata = test;
    loader.max_loads = 6;
    const char *error = NULL;
    int result = fvm_desktop_run_source_managed(options, source, &loader, &error);
    if (result != expected)
    {
        StyxMovieMonitorState current;
        styxmoviecontrolstate(test->monitor, &current);
        fprintf(stderr, "monitor run=%d expected=%d available=%d generation=%llu reason=%d live=%d error=%s\n",
            result, expected, current.available, current.generation, current.reason, styxclientalive(test->client), error ? error : "none");
    }
    check(result == expected && (result || !error), "actual monitored player result");
}


/* EN/RU: Verify generic dispatcher retained unrelated complete bound payloads after worker join.
 * Проверить unrelated complete bound payloads в generic dispatcher после worker join. */
static void mixed(Test *test)
{
    size_t offset = 0;
    unsigned found = 0;
    FVM_SBindings bindings;
    FVM_SBinding slots[4];
    fvm_sbindings_init(&bindings, slots, 4);
    while (offset < test->size)
    {
        FVM_SRecord record;
        size_t size;
        check(fvm_srecord_read(test->bytes+offset, test->size-offset, sizeof(test->bytes), &record, &size) == FVM_S_OK,
            "dispatcher lost raw framing");
        if (!record.tag)
        {
            size_t consumed;
            check(fvm_sbindings_apply(&bindings, test->bytes+offset, size, &consumed) == FVM_S_OK,
                "dispatcher lost binding");
        }
        else if (record.tag == 9)
        {
            FVM_SBinding binding;
            check(fvm_sbindings_lookup(&bindings, 9, &binding) == FVM_S_OK &&
                binding.type_size == sizeof("flashvm.test")-1 && !memcmp(binding.type, "flashvm.test", binding.type_size) &&
                record.size == 1 && record.payload[0] == 42, "unrelated native payload corrupted");
            found++;
        }
        offset += size;
    }
    check(found == 1, "unrelated S-record was stolen by movie reader");
}


/* EN/RU: Repeated immediate stop/free with caller session dropped first; source/client remain independent.
 * Повторять immediate stop/free после caller session drop; source/client независимы. */
static void immediate(StyxClient *client)
{
    for (unsigned i = 0; i < 24; i++)
    {
        Test test = {0};
        test.client = client;
        test.session = styxmoviesession(client);
        StyxMovieInput input = {received, &test, released};
        test.monitor = styxmoviecontrolstart(test.session, "ctl", &input);
        check(test.monitor != NULL, "immediate reader create");
        styxmoviesessionfree(test.session);
        styxmoviecontrolstop(test.monitor);
        check(styxmoviecontrolfree(test.monitor) && test.released == 1 && styxclientalive(client),
            "immediate stop/drop lifetime");
    }
}


/* EN/RU: Exercise one real TCP monitor lifetime; return successful outer fixture status.
 * Проверить один actual TCP monitor lifetime; вернуть success outer fixture. */
int movie_test_monitored(StyxClient *client, const FVM_DesktopOptions *options,
    const FVM_DesktopSource *source, const char *mode)
{
    Test test = {0};
    test.client = client;
    test.session = styxmoviesession(client);
    test.refuse = !strcmp(mode, "monitor-refuse");
    check(test.session != NULL, "receiver create");
    StyxMovieInput input = {received, &test, released};
    if (!strcmp(mode, "monitor-quota"))
    {
        check(!styxclientwatchable(client) && !styxmoviecontrolstart(test.session, "ctl", &input) &&
            !test.released && styxmovieavailable(test.session, NULL) == 0 && styxclientalive(client),
            "single-slot setup consumed session/context");
        styxmoviesessionfree(test.session);
        puts("PASS automatic control reader quota refusal before transport setup");
        return 0;
    }
    if (!strcmp(mode, "monitor-immediate"))
    {
        styxmoviesessionfree(test.session);
        immediate(client);
        puts("PASS repeated immediate control reader shutdown");
        return 0;
    }
    StyxMovieSession *partial = styxmoviesession(client);
    unsigned char prefix = 0x80;
    check(partial && styxmoviecontrol(partial, &prefix, 1) &&
        !styxmoviecontrolstart(partial, "ctl", &input) && !test.released &&
        styxmovieavailable(partial, NULL) == 0, "partial manual record was joined to a new stream");
    styxmoviesessionfree(partial);
    check(!styxmoviecontrolstart(test.session, "../escape", &input) &&
        !styxmoviecontrolstart(test.session, "movie", &input) && !test.released &&
        styxmovieavailable(test.session, NULL) == 0, "failed setup consumed context/receiver");
    movie_test_fail_thread_once();
    check(!styxmoviecontrolstart(test.session, "ctl", &input) && !test.released && !test.size &&
        styxmovieavailable(test.session, NULL) == 0, "thread-start refusal consumed input/session");
    test.monitor = styxmoviecontrolstart(test.session, "ctl", &input);
    check(test.monitor != NULL, "autonomous reader create");
    check(!styxmoviecontrolstart(test.session, "ctl", &input) &&
        !styxmoviecontrol(test.session, NULL, 0) && !test.released, "duplicate/manual feed stole input");
    int fd = styxmoviecontrolfd(test.monitor);
    check((fcntl(fd, F_GETFL) & O_NONBLOCK) && (fcntl(fd, F_GETFD) & FD_CLOEXEC), "readiness fd flags");
    if (test.refuse || !strcmp(mode, "monitor-crc") || !strcmp(mode, "monitor-eof"))
    {
        StyxMovieMonitorState state = await(&test, 0, 1);
        check(state.available == -1 && (state.reason == STYX_MOVIE_FAILED || state.reason == STYX_MOVIE_EOF),
            "refused/corrupt/EOF receiver remained available");
        run(&test, options, source, FVM_DESKTOP_EXIT_PLAYBACK);
    }
    else
    {
        (void)await(&test, 1, 0);
        int held = !strcmp(mode, "monitor-held");
        if (held)
        {
            styxmoviesessionfree(test.session);
        }
        if (!strcmp(mode, "monitor-changing"))
        {
            test.change = 1;
            run(&test, options, source, FVM_DESKTOP_EXIT_PLAYBACK);
        }
        run(&test, options, source, FVM_DESKTOP_EXIT_OK);
        if (!strcmp(mode, "monitor-stop"))
        {
            check(getchar() == 'g', "external pending-read gate");
            styxmoviecontrolstop(test.monitor);
            check(styxmovieavailable(test.session, NULL) == -1, "stop left stale authority");
        }
        else if (!strcmp(mode, "monitor-disconnect"))
        {
            styxclientshutdown(client);
        }
        else
        {
            if (strcmp(mode, "monitor-changing"))
            {
                command(&test, "replace");
                (void)await(&test, 2, 0);
                run(&test, options, source, FVM_DESKTOP_EXIT_OK);
            }
            command(&test, "close");
        }
        StyxMovieMonitorState state = await(&test, 0, 1);
        check(state.available == -1, "terminal monitor state still ready");
        run(&test, options, source, FVM_DESKTOP_EXIT_PLAYBACK);
        check(styxmoviecontrolfree(test.monitor) && test.released == 1, "monitor/context retirement");
        test.monitor = NULL;
        if (!strcmp(mode, "monitor-mixed"))
        {
            mixed(&test);
        }
        if (!held)
        {
            styxmoviesessionfree(test.session);
        }
        if (strcmp(mode, "monitor-disconnect"))
        {
            check(styxclientalive(client), "reader retirement shut down borrowed connection");
        }
        puts("PASS automatic control reader with actual player and native ownership");
        return 0;
    }
    check(styxmoviecontrolfree(test.monitor) && test.released == 1 && styxclientalive(client), "failed reader cleanup");
    styxmoviesessionfree(test.session);
    puts("PASS automatic control reader fail-closed acquisition");
    return 0;
}
