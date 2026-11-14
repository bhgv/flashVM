/* Copyright (c) 2026 bhgv
 * EN: Concurrent resolution/copy/rebind and stale-plan refusal without VM state.
 * RU: Параллельные resolve/copy/rebind и отказ stale-планам без VM state.
 */
#include "internal.h"
#include "observation.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DVM_NS_TEST_ROUNDS 2000 /* Independent stress iterations per native thread. */


/* EN/RU: Require result with an explicit diagnostic; failure exits the test.
 * Проверить result с явной диагностикой; при ошибке завершить тест. */
static void require(int result, const char *message)
{
    if (!result)
    {
        fprintf(stderr, "namespace test: %s\n", message);
        exit(2);
    }
}


/* EN/RU: Replace target from source in a quiescent or single-writer group.
 * Заменить target из source в группе без конкурирующих writers; ошибка фатальна. */
static void bind_path(DVM_Namespace *group, const char *source, const char *target)
{
    DVM_NamespacePlan from, on;
    require(dvm_namespace_plan(group, source, &from), "source plan");
    require(dvm_namespace_plan(group, target, &on), "target plan");
    require(dvm_namespace_bind(group, &from, &on) == 1, "bind commit");
}


/* EN/RU: Require one exact frozen native spelling from a borrowed group/path.
 * Проверить точный native path из заимствованных group/path; ошибка фатальна. */
static void expect_path(DVM_Namespace *group, const char *path, const char *expected)
{
    DVM_NamespacePlan plan;
    require(dvm_namespace_plan(group, path, &plan), "path plan");
    require(!strcmp(plan.native, expected), "resolved path mismatch");
}


/* EN: Validate component boundaries, aliases, copy isolation, stale commits,
 * filtered unmount and finite revisions before any concurrent use.
 * RU: До concurrency проверить границы компонентов, aliases, изоляцию копии,
 * stale commits, filtered unmount и исчерпание версий. */
static void geometry(void)
{
    DVM_Namespace *group = dvm_namespace_create("/native", "/cwd", NULL);
    DVM_NamespacePlan old, from, target;
    char oversized[DVM_NS_PATH_LIMIT + 1];
    require(group != NULL, "create");
    expect_path(group, ".././x//y/..", "/native/x");
    expect_path(group, "/../../x", "/native/x");
    expect_path(group, "/каталог/файл", "/native/каталог/файл");
    memset(oversized, 'x', sizeof(oversized));
    oversized[sizeof(oversized) - 1] = 0;
    require(!dvm_namespace_plan(group, oversized, &old), "oversized path accepted");
    require(!dvm_namespace_plan(group, "", &old), "empty path accepted");
    bind_path(group, "/a", "/view");
    expect_path(group, "/view/token", "/native/a/token");
    expect_path(group, "/view-other/token", "/native/view-other/token");
    bind_path(group, "/view", "/alias");
    require(dvm_namespace_plan(group, "/directory", &old), "old cwd plan");
    require(dvm_namespace_plan(group, "/b", &from), "old source plan");
    require(dvm_namespace_plan(group, "/view", &target), "old target plan");
    DVM_Namespace *copy = dvm_namespace_copy(group);
    require(copy != NULL, "copy");
    bind_path(group, "/b", "/view");
    require(dvm_namespace_chdir(group, &old) == 0, "stale chdir committed");
    require(dvm_namespace_bind(group, &from, &target) == 0, "stale bind committed");
    require(dvm_namespace_unmount(group, &target, NULL) == 0, "stale unmount committed");
    expect_path(copy, "/view/token", "/native/a/token");
    expect_path(group, "/alias/token", "/native/a/token");
    expect_path(group, "/view/token", "/native/b/token");
    bind_path(group, "/nested", "/view/sub");
    expect_path(group, "/view/sub/token", "/native/nested/token");
    require(dvm_namespace_plan(group, "/a", &from), "wrong source plan");
    require(dvm_namespace_plan(group, "/view", &target), "unmount target");
    require(dvm_namespace_unmount(group, &target, &from) == -1, "wrong source removed binding");
    require(dvm_namespace_plan(group, "/b", &from), "right source plan");
    require(dvm_namespace_unmount(group, &target, &from) == 1, "filtered unmount");
    expect_path(group, "/view/token", "/native/view/token");
    expect_path(group, "/view/sub/token", "/native/nested/token");
    bind_path(group, "/root", "/");
    expect_path(group, "/plain", "/native/root/plain");
    expect_path(group, "/alias/token", "/native/a/token");
    require(dvm_namespace_plan(group, "/directory", &old), "cwd plan");
    require(dvm_namespace_chdir(group, &old) == 1, "cwd commit");
    expect_path(group, "token", "/native/root/directory/token");
    group->version = UINT64_MAX; /* Unshared fixture only; simulate lifetime exhaustion. */
    require(dvm_namespace_plan(group, "/a", &from), "exhausted source plan");
    require(dvm_namespace_plan(group, "/alias", &target), "exhausted target plan");
    require(dvm_namespace_bind(group, &from, &target) == -1, "version wrapped in bind");
    require(dvm_namespace_chdir(group, &target) == -1, "version wrapped in chdir");
    require(dvm_namespace_unmount(group, &target, NULL) == -1, "version wrapped in unmount");
    expect_path(group, "/alias/token", "/native/a/token");
    dvm_namespace_release(copy);
    dvm_namespace_release(group);
}


/* EN: Alternate complete bindings; concurrent readers/copies must see one
 * frozen source, never mixed fields or freed records. Consume one group ref.
 * RU: Чередовать привязки; readers/copies видят один frozen source, без смешения
 * полей и freed records. Поглотить ссылку группы. */
static void *writer(void *argument)
{
    DVM_Namespace *group = argument;
    int index;
    for (index = 0; index < DVM_NS_TEST_ROUNDS; index++)
    {
        bind_path(group, index % 2 ? "/a" : "/b", "/view");
    }
    dvm_namespace_release(group);
    return NULL;
}


/* EN/RU: Resolve a shared group repeatedly; consume the thread's owning ref.
 * Многократно разрешить общую группу; поглотить ссылку потока. */
static void *reader(void *argument)
{
    DVM_Namespace *group = argument;
    DVM_NamespacePlan plan;
    int index;
    for (index = 0; index < DVM_NS_TEST_ROUNDS; index++)
    {
        DVM_NamespaceObservation state;
        unsigned char bytes[DVM_NS_OBSERVE_BYTES];
        uint64_t revision;
        int result;
        require(dvm_namespace_observe(group, NULL, 0, 0, 0, NULL, 0, &state) == DVM_NS_OBSERVED, "concurrent summary");
        revision = state.version;
        memset(bytes, 0, sizeof(bytes));
        result = dvm_namespace_observe(group, &revision, 0, DVM_NS_FIELD_SOURCE, 0, bytes, sizeof(bytes), &state);
        require(result == DVM_NS_OBSERVE_STALE || (result == DVM_NS_OBSERVED && (!strcmp((char *)bytes, "/native/a") ||
                                                                                 !strcmp((char *)bytes, "/native/b"))),
                "torn namespace observation");
        require(dvm_namespace_plan(group, "/view/token", &plan), "concurrent plan");
        require(!strcmp(plan.native, "/native/a/token") || !strcmp(plan.native, "/native/b/token"),
                "torn binding snapshot");
    }
    dvm_namespace_release(group);
    return NULL;
}


/* EN/RU: Copy during writes, then read independently; consume one group ref.
 * Копировать при writes, читать независимо; поглотить ссылку группы. */
static void *copier(void *argument)
{
    DVM_Namespace *group = argument;
    int index;
    for (index = 0; index < DVM_NS_TEST_ROUNDS; index++)
    {
        DVM_Namespace *copy = dvm_namespace_copy(group);
        DVM_NamespacePlan first, second;
        require(copy != NULL, "concurrent copy");
        require(dvm_namespace_plan(copy, "/view/token", &first), "copied plan");
        require(dvm_namespace_plan(copy, "/view/token", &second), "second copied plan");
        require(!strcmp(first.native, second.native), "detached group changed");
        require(!strcmp(first.native, "/native/a/token") || !strcmp(first.native, "/native/b/token"),
                "torn copied binding");
        dvm_namespace_release(copy);
    }
    dvm_namespace_release(group);
    return NULL;
}


/* EN: Check distinct initial import base, coherent copies, chdir alignment and
 * root clamping. Only caller owns these groups, no filesystem access.
 * RU: Проверить отдельный import base, согласованные копии, chdir alignment
 * и root clamping. Группами владеет только caller, без доступа к ФС. */
static void import_directory(void)
{
    DVM_Namespace *group = dvm_namespace_create("/native", "/cwd", "../package");
    DVM_NamespacePlan plan;
    require(group != NULL, "import group");
    expect_path(group, "provider.dis", "/native/cwd/provider.dis");
    require(dvm_namespace_module_plan(group, "provider.dis", &plan), "initial import plan");
    require(!strcmp(plan.native, "/native/package/provider.dis"), "initial import base changed");
    DVM_Namespace *copy = dvm_namespace_copy(group);
    require(copy != NULL, "import group copy");
    require(dvm_namespace_plan(group, "/second", &plan), "import chdir plan");
    require(dvm_namespace_chdir(group, &plan) == 1, "import chdir commit");
    require(dvm_namespace_module_plan(group, "provider.dis", &plan), "changed import plan");
    require(!strcmp(plan.native, "/native/second/provider.dis"), "chdir left old import directory");
    require(dvm_namespace_module_plan(copy, "provider.dis", &plan), "copied import plan");
    require(!strcmp(plan.native, "/native/package/provider.dis"), "import copy followed source change");
    dvm_namespace_release(copy);
    dvm_namespace_release(group);
    group = dvm_namespace_create("/native/package", "/", NULL);
    require(group && dvm_namespace_module_plan(group, "../../provider.dis", &plan), "rooted import plan");
    require(!strcmp(plan.native, "/native/package/provider.dis"), "import escaped lexical root");
    dvm_namespace_release(group);
}


/* EN: Check summary, exact slices, stale/invalid refusals and output bounds.
 * RU: Проверить summary, exact slices, stale/invalid refusal и output bounds.
 * Private native group only; no filesystem/managed state or implicit assertions. */
static void observations(void)
{
    DVM_Namespace *group = dvm_namespace_create("/native", "/cwd", "/package");
    DVM_NamespaceObservation state;
    unsigned char bytes[DVM_NS_OBSERVE_BYTES + 1];
    uint64_t revision;
    require(group != NULL, "observation create");
    bind_path(group, "/a", "/view");
    require(dvm_namespace_observe(group, NULL, 0, 0, 0, NULL, 0, &state) == DVM_NS_OBSERVED, "summary observation");
    require(state.bindings == 1 && state.references == 1 && !state.root_held, "summary values");
    revision = state.version;
    memset(bytes, 0xa5, sizeof(bytes));
    require(dvm_namespace_observe(group, &revision, 0, DVM_NS_FIELD_SOURCE, 2, bytes, 3, &state) == DVM_NS_OBSERVED,
            "source slice");
    require(state.length == strlen("/native/a") && state.copied == 3 && !memcmp(bytes, "ati", 3) && bytes[3] == 0xa5,
            "slice contents/bounds");
    require(dvm_namespace_observe(group, &revision, 0, DVM_NS_FIELD_ROOT, strlen("/native"), bytes, 1, &state) ==
                    DVM_NS_OBSERVED &&
                !state.copied,
            "end offset");
    require(dvm_namespace_observe(group, &revision, 0, DVM_NS_FIELD_ROOT, SIZE_MAX, bytes, 1, &state) ==
                DVM_NS_OBSERVE_INVALID,
            "offset overflow accepted");
    require(dvm_namespace_observe(group, &revision, SIZE_MAX, DVM_NS_FIELD_TARGET, 0, bytes, 1, &state) ==
                DVM_NS_OBSERVE_INVALID,
            "index overflow accepted");
    require(dvm_namespace_observe(group, &revision, 0, -1, 0, bytes, 1, &state) == DVM_NS_OBSERVE_INVALID,
            "negative field accepted");
    require(dvm_namespace_observe(group, &revision, 0, DVM_NS_FIELD_ROOT, 0, bytes, sizeof(bytes), &state) ==
                DVM_NS_OBSERVE_INVALID,
            "oversized chunk accepted");
    bind_path(group, "/b", "/view");
    memset(bytes, 0xa5, sizeof(bytes));
    require(dvm_namespace_observe(group, &revision, 0, DVM_NS_FIELD_SOURCE, 0, bytes, 3, &state) ==
                    DVM_NS_OBSERVE_STALE &&
                bytes[0] == 0xa5 && state.version > revision && !state.copied,
            "stale text leaked");
    dvm_namespace_release(group);
}


/* EN/RU: Run geometry then six simultaneous OS owners, join and release group.
 * Проверить geometry и шесть одновременных OS owners, join и освободить группу. */
int main(void)
{
    pthread_t owners[6];
    DVM_Namespace *group;
    int index;
    observations();
    geometry();
    import_directory();
    group = dvm_namespace_create("/native", "/", NULL);
    require(group != NULL, "stress create");
    bind_path(group, "/a", "/view");
    for (index = 0; index < 6; index++)
    {
        require(dvm_namespace_retain(group), "thread reference");
        require(!pthread_create(&owners[index],
                                NULL,
                                index == 0   ? writer
                                : index == 5 ? copier
                                             : reader,
                                group),
                "thread creation");
    }
    for (index = 0; index < 6; index++)
    {
        require(!pthread_join(owners[index], NULL), "thread join");
    }
    dvm_namespace_release(group);
    puts("PASS namespace geometry, stale plans, isolation and six native owners");
    return 0;
}
