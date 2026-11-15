/* Copyright (c) 2026 bhgv
 * EN: Stable subscriptions across namespace churn, partial allocation failure and epoch exhaustion.
 * RU: Stable subscriptions при namespace churn, partial allocation failure и epoch exhaustion.
 */
#include "internal.h"
#include "sysevents_internal.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void test_sysevents_calloc_failure(size_t size, unsigned match); /* Native test-only one-shot allocator failure. */
unsigned test_sysevents_calloc_failures(void); /* Actual rejection evidence, not inferred from a missing watch. */


/* EN/RU: Fail with a native label, independent of conditional assertion settings.
 * Явный native failure с label, без conditional assertions. */
static void require(int ok, const char *label)
{
    if (!ok)
    {
        fprintf(stderr, "sysevents subscriptions: %s\n", label);
        abort();
    }
}


/* EN/RU: Native source state survives until the final subscription/factory release.
 * Native source state живёт до final subscription/factory release. */
typedef struct Source
{
    unsigned destroyed; /* Final destructor count, updated synchronously in these scenarios. */
} Source;


/* EN/RU: Automatic reconciliation may not query source metadata.
 * Automatic reconciliation не должна запрашивать source metadata. */
static FVM_Result info(void *state, const char *path, FVM_FileInfo *output)
{
    (void)state;
    (void)path;
    (void)output;
    require(0, "source stat during reconciliation");
    return FVM_ERROR;
}


/* EN/RU: Automatic reconciliation may not open source data or ask a guest to act.
 * Automatic reconciliation не должна открывать source data или исполнять guest. */
static FVM_Result open_source(void *state, const char *path, const char *mode, DVM_VfsHandle *output)
{
    (void)state;
    (void)path;
    (void)mode;
    (void)output;
    require(0, "source open during reconciliation");
    return FVM_ERROR;
}


/* EN/RU: Count final native factory destruction without freeing stack-owned state.
 * Посчитать final native factory destruction без освобождения stack-owned state. */
static void destroy(void *state)
{
    Source *source = state;
    source->destroyed++;
}
static const DVM_ProviderOps ops = {info, open_source, destroy, 0, NULL}; /* No source I/O is available to observers. */


/* EN/RU: Native startup-style mount through the actual revision-checked operation.
 * Native mount через actual revision-checked operation, без guest registration. */
static void mount(DVM_Namespace *group, DVM_Provider *source, const char *on)
{
    DVM_NamespacePlan plan;
    require(dvm_namespace_plan(group, on, &plan) &&
            dvm_namespace_mount_provider(group, &plan, source, "/private-source") == 1, "provider mount");
}


/* EN/RU: Publish a real accepted source event from the test's native source owner.
 * Опубликовать реальное source event из native source owner теста. */
static void publish(DVM_Provider *source, uint64_t generation)
{
    DVM_ChangeEvent event = {0, generation, DVM_CHANGE_CONTENT, 0, 0};
    event.session = dvm_changes_session(dvm_provider_changes(source));
    require(dvm_changes_notify(dvm_provider_changes(source), &event) == 1, "source report");
}


/* EN/RU: Change only cwd; subscriptions must survive without source/tap allocation.
 * Менять только cwd; subscriptions должны сохраниться без source/tap allocation. */
static void chdir(DVM_Namespace *group)
{
    DVM_NamespacePlan plan;
    require(dvm_namespace_plan(group, "/elsewhere", &plan) && dvm_namespace_chdir(group, &plan) == 1,
            "native namespace mutation");
}


/* EN/RU: Locate a native subscribed identity after synchronous mutation completion.
 * Найти subscribed identity после synchronous mutation; returned pointer borrowed. */
static DVM_SysWatch *find(DVM_SysEvents *events, DVM_Provider *source)
{
    DVM_SysWatch *watch;
    for (watch = events->watches; watch; watch = watch->next)
    {
        if (watch->provider == source)
        {
            return watch;
        }
    }
    return NULL;
}


/* EN/RU: Drain one whole record per call; return VFS count and optional last generation.
 * Прочитать по одной записи; вернуть VFS count и optional last generation. */
static unsigned drain(DVM_VfsHandle *fd, uint64_t *last)
{
    unsigned found = 0;
    for (unsigned index = 0; index < DVM_SYSEVENT_LIMIT + 3; index++)
    {
        char text[DVM_SYSEVENT_FRAME_BYTES];
        unsigned kind, classes;
        uint64_t binding, session, generation;
        DVM_IoResult result = dvm_vfs_read_chunk(fd, text, sizeof(text) - 1);
        if (result.stop == DVM_IO_AGAIN)
        {
            require(!result.count, "empty inbox acknowledged bytes");
            return found;
        }
        require(result.status == FVM_OK && result.count && (result.flags & DVM_IO_RECORD), "record delivery");
        text[result.count] = 0;
        require(sscanf(text, "system %u %u %" SCNx64 " %" SCNx64 " %" SCNx64,
                &kind, &classes, &binding, &session, &generation) == 5, "experimental envelope");
        if (kind == DVM_SYSEVENT_VFS)
        {
            found++;
            if (last)
            {
                *last = generation;
            }
        }
    }
    require(0, "inbox failed to drain");
    return 0;
}


/* EN/RU: Reuse preserves exact context/tap/watermarks and avoids duplicate initial VFS reports.
 * Reuse сохраняет context/tap/watermarks, не выдаёт повторные initial VFS reports. */
static void unchanged(DVM_Namespace *group, DVM_SysEvents *events, DVM_Provider *source, DVM_VfsHandle *fd)
{
    DVM_SysWatch *watch = find(events, source);
    DVM_NoticeTap *tap = watch->tap;
    unsigned before = test_sysevents_calloc_failures();
    publish(source, 2);
    drain(fd, NULL);
    test_sysevents_calloc_failure(sizeof(DVM_SysWatch), 1);
    chdir(group);
    test_sysevents_calloc_failure(0, 0);
    require(test_sysevents_calloc_failures() == before && find(events, source) == watch &&
            watch->tap == tap && watch->last_generation == 2 && events->captured,
            "cwd recreated/dropped a stable subscription");
    require(drain(fd, NULL) == 0, "cwd fabricated initial provider reports");
    publish(source, 3);
    require(drain(fd, NULL) == 1, "reused source no longer pushes");
}


/* EN/RU: Failure of a new watch/tap must not drop an unrelated existing subscriber.
 * Отказ новой watch/tap не должен отключать существующий unrelated subscriber. */
static void partial_failure(DVM_Namespace *group, DVM_SysEvents *events, DVM_Provider *existing,
                              DVM_Provider *added, DVM_VfsHandle *fd, int tap_failure)
{
    DVM_SysWatch *watch = find(events, existing);
    DVM_NoticeTap *tap = watch->tap;
    unsigned before = test_sysevents_calloc_failures();
    uint64_t last = 0;
    /* Native mount allocations: binding, namespace record, watch, then notice tap. */
    test_sysevents_calloc_failure(tap_failure ? 0 : sizeof(DVM_SysWatch), tap_failure ? 4 : 1);
    mount(group, added, tap_failure ? "/tap-failure" : "/watch-failure");
    test_sysevents_calloc_failure(0, 0);
    require(test_sysevents_calloc_failures() == before + 1 && !events->captured && events->resync &&
            find(events, existing) == watch && watch->tap == tap && !find(events, added),
            "partial failure lost stable source or hid subscription loss");
    publish(existing, tap_failure ? 5 : 4);
    require(drain(fd, &last) == 1 && last == (tap_failure ? 5U : 4U), "old source stopped during partial failure");
    chdir(group);
    require(events->captured && find(events, existing) == watch && find(events, added),
            "later native mutation did not repair missing subscription");
    require(drain(fd, NULL) == 1, "repair repeated unchanged provider initialization");
    publish(added, 2);
    require(drain(fd, NULL) == 1, "repaired source did not push automatically");
}


/* EN/RU: Removing one binding must detach it without rebuilding the others.
 * Удаление binding отключает её без пересоздания остальных подписок. */
static void removal(DVM_Namespace *group, DVM_SysEvents *events, DVM_Provider *removed,
                       DVM_Provider *stable, DVM_VfsHandle *fd)
{
    DVM_NamespacePlan target;
    DVM_SysWatch *watch = find(events, stable);
    unsigned before = test_sysevents_calloc_failures();
    uint64_t last = 0;
    test_sysevents_calloc_failure(sizeof(DVM_SysWatch), 1);
    require(dvm_namespace_plan(group, "/watch-failure", &target) &&
            dvm_namespace_unmount(group, &target, NULL) == 1, "provider removal");
    test_sysevents_calloc_failure(0, 0);
    require(test_sysevents_calloc_failures() == before && !find(events, removed) &&
            find(events, stable) == watch && events->captured, "removal rebuilt peers or retained old source");
    publish(removed, 3);
    publish(stable, 6);
    require(drain(fd, &last) == 1 && last == 6, "removed source delivered or stable source stopped");
}


/* EN/RU: Same path/provider with a new binding ID must create a fresh subscription.
 * Same path/provider с новой binding ID должны получить новую subscription. */
static void replacement(DVM_Namespace *group, DVM_SysEvents *events, DVM_Provider *source, DVM_VfsHandle *fd)
{
    uint64_t identity = find(events, source)->identity;
    mount(group, source, "/stable");
    require(find(events, source) && find(events, source)->identity != identity && events->captured,
            "replacement reused an obsolete binding identity");
    require(drain(fd, NULL) == 1, "replacement missed new-binding snapshot or repeated peer snapshots");
    publish(source, 7);
    require(drain(fd, NULL) == 1, "replacement source did not push");
}


/* EN/RU: Reused subscriptions preserve reconnect epochs and reject stale reports.
 * Reused subscriptions сохраняют reconnect epochs и отвергают stale reports. */
static void reconnect(DVM_Namespace *group, DVM_SysEvents *events, DVM_Provider *source, DVM_VfsHandle *fd)
{
    DVM_SysWatch *watch = find(events, source);
    DVM_ChangeEvent stale = {1, 100, DVM_CHANGE_CONTENT, 0, 0};
    require(dvm_changes_reconnect(dvm_provider_changes(source), 1) == 2, "source reconnect");
    require(drain(fd, NULL) == 1 && watch->last_session == 2 && watch->last_generation == 1,
            "reused source lost reconnect epoch");
    chdir(group);
    require(find(events, source) == watch && drain(fd, NULL) == 0, "post-reconnect cwd recreated watch");
    require(dvm_changes_notify(dvm_provider_changes(source), &stale) == 0 && drain(fd, NULL) == 0,
            "old source session replaced reconnect state");
    publish(source, 2);
    require(drain(fd, NULL) == 1 && watch->last_generation == 2, "new epoch source stopped pushing");
}


/* EN/RU: Exhausted scope may not retain old source authority or fabricate owner exit.
 * Exhausted scope не должен сохранять old source authority или изображать owner exit. */
static void exhaustion(DVM_Namespace *group, DVM_SysEvents *events, DVM_Provider *source, DVM_VfsHandle *fd)
{
    DVM_SysWatch *watch;
    fvm_port_sync_lock(events->lock);
    events->epoch = UINT64_MAX;
    for (watch = events->watches; watch; watch = watch->next)
    {
        watch->epoch = UINT64_MAX;
    }
    fvm_port_sync_unlock(events->lock);
    chdir(group);
    require(events->scope_closed && events->resync && !events->watches && !events->retired &&
            events->epoch == UINT64_MAX, "scope exhaustion retained old subscriptions/wrapped/retired owner");
    publish(source, 8);
    require(drain(fd, NULL) == 0, "exhausted scope leaked detached source reports");
    require(!dvm_sysevents_group(events, group), "exhausted scope replaced namespace");
}


/* EN/RU: Actual native namespace changes, fault injection and cleanup; no receiver execution.
 * Actual native mutations/fault injection/cleanup без исполнения receiver. */
void test_sysevents_subscriptions(void)
{
    Source a = {0}, b = {0}, c = {0};
    DVM_Namespace *group = dvm_namespace_create("/", "/", NULL);
    DVM_Provider *first = dvm_provider_create(&ops, &a), *second = dvm_provider_create(&ops, &b),
                 *third = dvm_provider_create(&ops, &c);
    DVM_SysEvents *events;
    DVM_VfsHandle fd = {0};
    require(group && first && second && third, "construction");
    mount(group, first, "/stable");
    events = dvm_sysevents_create(group);
    require(events && dvm_sysevents_open(events, "r", &fd) == FVM_OK, "inbox construction");
    require(drain(&fd, NULL) == 1, "initial source snapshot");
    unchanged(group, events, first, &fd);
    partial_failure(group, events, first, second, &fd, 0);
    partial_failure(group, events, first, third, &fd, 1);
    removal(group, events, second, first, &fd);
    replacement(group, events, first, &fd);
    reconnect(group, events, first, &fd);
    exhaustion(group, events, first, &fd);
    dvm_sysevents_retire(events);
    dvm_vfs_close(&fd);
    dvm_sysevents_release(events);
    dvm_namespace_release(group);
    dvm_provider_release(first);
    dvm_provider_release(second);
    dvm_provider_release(third);
    require(a.destroyed == 1 && b.destroyed == 1 && c.destroyed == 1, "partial/exhausted subscription lifetime leak");
    puts("PASS stable subscriptions survive cwd and watch/tap allocation failure, native retry, no duplicate initialization, fail-closed epoch exhaustion");
}
