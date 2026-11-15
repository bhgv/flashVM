/* Copyright (c) 2026 bhgv
 * EN: Automatic owner inboxes, namespace isolation, overflow and retained device FDs.
 * RU: Автоматические owner inboxes, namespace isolation, overflow и retained FD.
 */
#include "sysevents_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void test_native_notices(void); /* Native sender/callback lifetime acceptance, no VM required. */
void test_sysevents_queue(void); /* Atomic shared inbox, failure and concurrent-reader acceptance. */
void test_sysevents_race(void); /* Source publication versus native namespace churn. */
void test_sysevents_subscriptions(void); /* Stable reuse, native allocation faults and scope exhaustion. */
static unsigned destroyed; /* Final source release count after bindings/engine watches retire. */


/* EN/RU: Explicit native test failure, without conditional assertions.
 * Явный test failure, без conditional assertions. */
static void require(int ok, const char *label)
{
    if (!ok)
    {
        fprintf(stderr, "sysevents: %s\n", label);
        exit(2);
    }
}


/* EN/RU: Automatic invalidation tracking must never query source attributes.
 * Automatic tracking не должен запрашивать source attributes. */
static FVM_Result forbidden_info(void *state, const char *path, FVM_FileInfo *output)
{
    (void)state;
    (void)path;
    (void)output;
    require(0, "automatic observer queried metadata");
    return FVM_ERROR;
}


/* EN/RU: Automatic invalidation tracking must never open source data.
 * Automatic tracking не должен открывать source data. */
static FVM_Result forbidden_open(void *state, const char *path, const char *mode, DVM_VfsHandle *output)
{
    (void)state;
    (void)path;
    (void)mode;
    (void)output;
    require(0, "automatic observer opened source");
    return FVM_ERROR;
}


/* EN/RU: Count final source destruction, state is borrowed stack storage.
 * Посчитать final destruction; state — borrowed stack storage. */
static void destroy(void *state)
{
    (void)state;
    destroyed++;
}
static const DVM_ProviderOps source_ops = {forbidden_info, forbidden_open, destroy, 0, NULL}; /* No metadata access granted. */


/* EN/RU: Native startup mount, without path validation or guest registration.
 * Native startup mount, без path validation и guest registration. */
static void mount(DVM_Namespace *group, DVM_Provider *source, const char *on)
{
    DVM_NamespacePlan plan;
    require(dvm_namespace_plan(group, on, &plan) &&
            dvm_namespace_mount_provider(group, &plan, source, "/private-source") == 1, "mount");
}


/* EN/RU: Publish an actual provider change; no observer initialization is involved.
 * Опубликовать change; observer initialization не требуется. */
static void change(DVM_Provider *source, uint64_t generation, unsigned kinds)
{
    DVM_ChangeEvent event = {1, generation, kinds, 0, 0};
    require(dvm_changes_notify(dvm_provider_changes(source), &event) == 1, "provider notification");
}


/* EN/RU: Drain complete records through the actual readonly stream; AGAIN is not EOF.
 * Return number of selected type/class records. Прочитать records через stream;
 * AGAIN не EOF. Вернуть число records с выбранным type/classes. */
static unsigned drain(DVM_VfsHandle *fd, unsigned selected, unsigned classes)
{
    char text[DVM_SYSEVENT_FRAME_BYTES];
    unsigned count = 0, index;
    for (index = 0; index < DVM_SYSEVENT_LIMIT + 5; index++)
    {
        unsigned kind, mask;
        DVM_IoResult result = dvm_vfs_read_chunk(fd, text, sizeof(text) - 1);
        if (result.status != FVM_OK && result.stop == DVM_IO_AGAIN && !result.count)
        {
            return count;
        }
        require(result.status == FVM_OK && result.count && result.count < sizeof(text) &&
                (result.flags & DVM_IO_RECORD), "unexpected error/EOF or missing record boundary");
        text[result.count] = 0;
        require(strchr(text, '\n') == text + result.count - 1, "batched records");
        require(sscanf(text, "system %u %u", &kind, &mask) == 2, "malformed frame");
        if (kind == selected && mask == classes)
        {
            count++;
        }
    }
    require(0, "mailbox did not become idle");
    return 0;
}


/* EN/RU: Engine binding occurs before any device open. Verify distinct inboxes
 * for shared namespace, old-scope purge, automatic mount tracking and bounded loss.
 * Engine работает до open device. Проверить per-thread inbox, scope purge и overflow. */
int main(void)
{
    int state = 0;
    DVM_Namespace *group = dvm_namespace_create("/", "/", NULL);
    DVM_Namespace *hidden = dvm_namespace_create("/jail", "/", NULL);
    DVM_Provider *source = dvm_provider_create(&source_ops, &state), *device = dvm_sysevents_device();
    DVM_SysEvents *first, *second;
    DVM_VfsHandle a = {0}, b = {0};
    DVM_NamespacePlan target;
    DVM_IoResult outcome;
    char prefix[5], record[DVM_SYSEVENT_FRAME_BYTES];
    unsigned generation;
    test_native_notices();
    test_sysevents_queue();
    test_sysevents_subscriptions();
    test_sysevents_race();
    require(group && hidden && source && device, "construction");
    mount(group, source, "/visible");
    mount(group, device, "/dev/sysevents");
    first = dvm_sysevents_create(group);
    second = dvm_sysevents_create(group);
    require(first && second, "inboxes");
    change(source, 2, DVM_CHANGE_RIGHTS | DVM_CHANGE_CONTENT);
    require(first->pending >= 2 && second->pending >= 2 && first->watches &&
            first->watches->last_generation == 2 && second->watches->last_generation == 2,
            "tracking required guest device open/registration");
    dvm_sysevents_bind(first);
    require(dvm_provider_open(device, ".", "r", &a) == FVM_OK, "first contextual open");
    dvm_sysevents_bind(second);
    require(dvm_provider_open(device, ".", "r", &b) == FVM_OK, "second contextual open");
    require(drain(&a, DVM_SYSEVENT_VFS, DVM_CHANGE_RIGHTS | DVM_CHANGE_CONTENT) == 1 && second->pending,
            "first consumer cleared second inbox");
    drain(&b, DVM_SYSEVENT_NAMESPACE, DVM_CHANGE_ALL);
    change(source, 3, DVM_CHANGE_CONNECTION);
    dvm_sysevents_bind(first);
    dvm_sysevents_bind(second);
    outcome = dvm_vfs_read_chunk(&a, prefix, sizeof(prefix));
    require(outcome.status != FVM_OK && !outcome.count, "short buffer consumed/split record");
    require(dvm_sysevents_group(first, hidden), "new namespace scope");
    require(drain(&a, DVM_SYSEVENT_NAMESPACE, DVM_CHANGE_ALL) == 1, "partial old path survived namespace switch");
    change(source, 4, DVM_CHANGE_RIGHTS);
    dvm_sysevents_bind(first);
    require(drain(&a, DVM_SYSEVENT_VFS, DVM_CHANGE_RIGHTS) == 0, "hidden provider still tracked");
    for (generation = 5; generation < 80; generation++)
    {
        change(source, generation, DVM_CHANGE_CONTENT);
    }
    require(second->pending <= DVM_SYSEVENT_LIMIT && second->resync, "unbounded/silent mailbox overflow");
    require(drain(&b, DVM_SYSEVENT_RESYNC, DVM_CHANGE_ALL) == 1, "overflow duplicated/lost resync signal");
    require(dvm_namespace_plan(group, "/visible", &target) && dvm_namespace_unmount(group, &target, NULL) == 1,
            "source unmount");
    dvm_sysevents_bind(second);
    require(drain(&b, DVM_SYSEVENT_NAMESPACE, DVM_CHANGE_ALL) == 1, "unmount not automatically tracked");
    dvm_provider_release(source);
    require(destroyed == 1, "stale engine subscription retained source forever");
    dvm_sysevents_bind(NULL);
    dvm_sysevents_retire(first);
    dvm_sysevents_release(first);
    outcome = dvm_vfs_read_chunk(&a, record, sizeof(record));
    require(outcome.status == FVM_OK && outcome.count && (outcome.flags & DVM_IO_RECORD), "retired FD lost exit record");
    outcome = dvm_vfs_read_chunk(&a, record, sizeof(record));
    require(outcome.status == FVM_OK && !outcome.count && outcome.stop == DVM_IO_EOF, "retired FD missing EOF");
    dvm_vfs_close(&a);
    dvm_sysevents_retire(second);
    dvm_sysevents_release(second);
    dvm_vfs_close(&b);
    dvm_namespace_release(group);
    dvm_namespace_release(hidden);
    dvm_provider_release(device);
    puts("PASS automatic sysevents before guest open, independent inboxes, namespace hiding, overflow, unmount and retained FD");
    return 0;
}
