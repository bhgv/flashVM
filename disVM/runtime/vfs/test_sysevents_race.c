/* Copyright (c) 2026 bhgv
 * EN: Real source push races namespace replacement and subscription retirement.
 * RU: Реальный source push параллельно namespace replacement и subscription retirement.
 */
#include "sysevents_internal.h"
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RACE_REPORTS 2000U /* Accepted contiguous reports from a dedicated native source. */
#define RACE_REBINDS 200U /* Namespace replacements while the source owns its native execution thread. */


/* EN/RU: Stack-owned race inputs live until both native producers are joined.
 * Stack-owned race inputs живут до join обоих native producers. */
typedef struct Race
{
    DVM_Namespace *visible; /* Shared namespace repeatedly replacing one provider binding. */
    DVM_Namespace *hidden; /* Empty scope that must reject all detached-provider callbacks. */
    DVM_Provider *source; /* Retained journal-reporting native source factory. */
    DVM_SysEvents *moving; /* Independent inbox switched between visible/hidden scopes. */
    unsigned destroyed; /* Final source destruction, inspected only after sender joins. */
} Race;


/* EN/RU: Native failure diagnostic, safe to call from either producer.
 * Явный native failure diagnostic, допустимый в обоих producers. */
static void require(int ok, const char *label)
{
    if (!ok)
    {
        fprintf(stderr, "sysevents race: %s\n", label);
        abort();
    }
}


/* EN/RU: No automatic subscription operation may query source metadata.
 * Automatic subscription operation не должна запрашивать source metadata. */
static FVM_Result forbidden_info(void *state, const char *path, FVM_FileInfo *output)
{
    (void)state;
    (void)path;
    (void)output;
    require(0, "automatic source stat");
    return FVM_ERROR;
}


/* EN/RU: No automatic subscription operation may open provider data.
 * Automatic subscription operation не должна открывать provider data. */
static FVM_Result forbidden_open(void *state, const char *path, const char *mode, DVM_VfsHandle *output)
{
    (void)state;
    (void)path;
    (void)mode;
    (void)output;
    require(0, "automatic source open");
    return FVM_ERROR;
}


/* EN/RU: Count final source destruction; only this callback mutates the counter.
 * Посчитать final source destruction; counter меняется только здесь. */
static void destroy(void *state)
{
    Race *race = state;
    race->destroyed++;
}
static const DVM_ProviderOps ops = {forbidden_info, forbidden_open, destroy, 0, NULL}; /* No guest registration or metadata service. */


/* EN/RU: Replace an ordinary native binding; callbacks run after the namespace lock.
 * Сменить обычный native binding; callbacks выполняются после namespace lock. */
static void mount(Race *race)
{
    DVM_NamespacePlan plan;
    require(dvm_namespace_plan(race->visible, "/moving", &plan) &&
            dvm_namespace_mount_provider(race->visible, &plan, race->source, "/native-source") == 1,
            "binding replacement");
}


/* EN/RU: Publish a real source report with the supplied unique generation.
 * Опубликовать реальный source report с заданной уникальной generation. */
static void publish(Race *race, uint64_t generation)
{
    DVM_ChangeEvent event = {1, generation, DVM_CHANGE_CONTENT, 0, 0};
    require(dvm_changes_notify(dvm_provider_changes(race->source), &event) == 1, "source report admission");
}


/* EN/RU: Source has no receiving guest execution or registration dependencies.
 * Source не зависит от receiving guest execution или registration. Return NULL. */
static void *sender(void *state)
{
    Race *race = state;
    for (unsigned index = 2; index <= RACE_REPORTS + 1; index++)
    {
        publish(race, index);
    }
    return NULL;
}


/* EN/RU: Rebind while a second inbox alternates namespace groups, ending hidden.
 * Rebind параллельно смене scope второго inbox; final scope — hidden. Return NULL. */
static void *mutator(void *state)
{
    Race *race = state;
    for (unsigned index = 0; index < RACE_REBINDS; index++)
    {
        mount(race);
        require(dvm_sysevents_group(race->moving, index % 2 ? race->hidden : race->visible),
                "moving inbox namespace switch");
    }
    return NULL;
}


/* EN/RU: Drain finite copied notices after sender joins; return last VFS generation.
 * Прочитать finite copied notices после join; вернуть last VFS generation.
 * hidden forbids every VFS/path exposure, independent from native sender timing. */
static uint64_t drain(DVM_VfsHandle *fd, int hidden)
{
    char text[DVM_SYSEVENT_FRAME_BYTES];
    uint64_t last = 0;
    for (unsigned index = 0; index < DVM_SYSEVENT_LIMIT + 3; index++)
    {
        unsigned kind, classes;
        uint64_t binding, session, generation;
        DVM_IoResult result = dvm_vfs_read_chunk(fd, text, sizeof(text) - 1);
        if (result.stop == DVM_IO_AGAIN)
        {
            require(!result.count, "empty inbox count");
            return last;
        }
        require(result.status == FVM_OK && result.count && (result.flags & DVM_IO_RECORD), "finite record read");
        text[result.count] = 0;
        require(sscanf(text, "system %u %u %" SCNx64 " %" SCNx64 " %" SCNx64,
                &kind, &classes, &binding, &session, &generation) == 5, "record envelope");
        require(!hidden || (kind == DVM_SYSEVENT_NAMESPACE && !binding && !session), "hidden source notification leaked");
        if (kind == DVM_SYSEVENT_VFS)
        {
            last = generation;
        }
    }
    require(0, "bounded mailbox did not drain");
    return 0;
}


/* EN/RU: Test actual source delivery/namespace teardown races without VM execution.
 * Проверить source delivery/namespace teardown races без VM execution. */
void test_sysevents_race(void)
{
    Race race = {0};
    DVM_SysEvents *stable;
    DVM_VfsHandle a = {0}, b = {0};
    pthread_t source_thread, namespace_thread;
    race.visible = dvm_namespace_create("/", "/", NULL);
    race.hidden = dvm_namespace_create("/hidden", "/", NULL);
    race.source = dvm_provider_create(&ops, &race);
    require(race.visible && race.hidden && race.source, "construction");
    mount(&race);
    stable = dvm_sysevents_create(race.visible);
    race.moving = dvm_sysevents_create(race.visible);
    require(stable && race.moving && dvm_sysevents_open(stable, "r", &a) == FVM_OK &&
            dvm_sysevents_open(race.moving, "r", &b) == FVM_OK, "inbox construction");
    require(!pthread_create(&source_thread, NULL, sender, &race) &&
            !pthread_create(&namespace_thread, NULL, mutator, &race), "native producer creation");
    require(!pthread_join(source_thread, NULL) && !pthread_join(namespace_thread, NULL), "native producer join");
    publish(&race, RACE_REPORTS + 2);
    require(stable->pending <= DVM_SYSEVENT_LIMIT && race.moving->pending <= DVM_SYSEVENT_LIMIT,
            "race exceeded inbox limit");
    require(drain(&a, 0) == RACE_REPORTS + 2, "final live source report lost");
    require(drain(&b, 1) == 0 && race.moving->watches == NULL, "detached source retained or visible");
    dvm_sysevents_retire(stable);
    dvm_sysevents_retire(race.moving);
    dvm_vfs_close(&a);
    dvm_vfs_close(&b);
    dvm_sysevents_release(stable);
    dvm_sysevents_release(race.moving);
    dvm_namespace_release(race.visible);
    dvm_namespace_release(race.hidden);
    dvm_provider_release(race.source);
    require(race.destroyed == 1, "in-flight source subscription lifetime leak");
    puts("PASS source push concurrent with provider rebind and per-inbox namespace replacement, hidden scope and final cleanup");
}
