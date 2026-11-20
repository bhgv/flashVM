/* Copyright (c) 2026 bhgv
 * EN: Mount the two live Styx file grants before the real guest starts.
 * RU: Смонтировать два живых файла Styx до старта настоящего гостя.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <inttypes.h>
#include "vfs/export.h"
#include "vfs/program_view.h"
#include "vfs/loader.h"
#include "vfs/module_adapter.h"
#include "vfs/launch/runtime.h"

#include "guest_mount.h"
#include "../../scalar_interface.h"
#include "../../unibus_inbox.h"


/* EN/RU: Read a bounded fixture image and return an owned immutable provider.
 * Прочитать ограниченный тестовый образ и вернуть неизменяемый провайдер. */
DVM_Provider *scalar_local_image(const char *path)
{
    FILE *file = fopen(path, "rb");
    unsigned char *bytes = malloc(DVM_MODULE_IMAGE_LIMIT);
    size_t size = file && bytes ? fread(bytes, 1, DVM_MODULE_IMAGE_LIMIT, file) : 0;
    int valid = file && !ferror(file);
    if (file)
    {
        valid = fclose(file) == 0 && valid;
    }
    DVM_Provider *adapter = valid ? dvm_module_adapter(bytes, size) : NULL;
    free(bytes);
    return adapter;
}


/* EN/RU: Construct an owned profile with one published scalar offer.
 * Создать собственный профиль с одним предложением скалярного интерфейса. */
DVM_Provider *scalar_local_profile(void)
{
    DVM_Provider *profile = styxunibusinbox(61);
    if (!profile || styxscalaroffer(profile, 61, "compute", SCALAR_FIXTURE_SIGNATURE) != DVM_LAUNCH_DELIVERED)
    {
        dvm_provider_release(profile);
        return NULL;
    }
    return profile;
}


/* EN/RU: Mount the supplied original with its local adapter and bind the ordinary name.
 * Смонтировать исходный файл с локальным адаптером и привязать обычное имя. */
static int publish_adapter(DVM_Namespace *group, DVM_Provider *original, DVM_Provider *profile,
    DVM_Provider *writer, DVM_Provider *reader)
{
    DVM_ResourcePlan plain = {0};
    DVM_Provider *ordinary = dvm_resource_plan(group, "/client.dis", 0, &plain) ?
        dvm_export_native(&plain, 0) : NULL;
    dvm_resource_close(&plain);
    static const char original_bytes[] = "original-image"; /* Distinct data-side fixture image. */
    DVM_Provider *fallback = original ? NULL : dvm_module_adapter(original_bytes, sizeof(original_bytes) - 1);
    DVM_Provider *local_profile = profile ? NULL : scalar_local_profile();
    DVM_Provider *publication = original ?
        dvm_module_publish_scalar_files(original, profile, 61, writer, 4294967295UL, reader, 64) :
        dvm_module_publish_scalar(fallback, local_profile, 61, 4294967295UL, 64);
    dvm_provider_release(local_profile);
    DVM_View *tree = dvm_view_create(NULL, NULL, NULL);
    DVM_Provider *modules = NULL;
    if (tree && publication && dvm_view_map(tree, "/published.dis", publication, ".",
            FVM_ACCESS_READ | FVM_ACCESS_EXEC, 0) &&
        dvm_view_map(tree, "/readonly.dis", publication, ".", FVM_ACCESS_READ, 0))
    {
        modules = dvm_view_freeze(tree);
    }
    else
    {
        dvm_view_discard(tree);
    }
    int mounted = ordinary && dvm_stack_mount(group, ordinary, "/ordinary.dis", 0, 0) == 1 &&
        modules && dvm_stack_mount(group, modules, "/library", 0, 0) == 1;
    dvm_provider_release(modules);
    dvm_provider_release(publication);
    dvm_provider_release(fallback);
    dvm_provider_release(ordinary);
    if (!mounted)
    {
        return 0;
    }
    DVM_ResourcePlan source = {0};
    DVM_ResourcePlan target = {0};
    int bound = dvm_resource_plan(group, "/library/published.dis", 0, &source) &&
        dvm_resource_plan(group, "/implementation.dis", 0, &target) &&
        dvm_stack_bind(&source, &target, 0, 0) == 1;
    dvm_resource_close(&target);
    dvm_resource_close(&source);
    return bound;
}


/* EN/RU: Mount the borrowed routes in a private test namespace with local code.
 * Смонтировать заимствованные маршруты в отдельном VFS с локальным кодом. */
int scalar_runtime_run(int argc, char **argv, DVM_Provider *writer, DVM_Provider *reader, DVM_Provider *original, DVM_Provider *profile, int owner)
{
    char cwd[DVM_NS_PATH_LIMIT];
    if (!getcwd(cwd, sizeof(cwd)))
    {
        exit(2);
    }
    DVM_View *view = dvm_view_create(NULL, NULL, NULL);
    if (!view || !dvm_view_map(view, owner ? "/reply" : "/request", writer, ".", FVM_ACCESS_WRITE, 0) ||
        !dvm_view_map(view, owner ? "/request" : "/reply", reader, ".", FVM_ACCESS_READ, 0))
    {
        exit(2);
    }
    DVM_Provider *routes = dvm_view_freeze(view);
    DVM_LaunchHandoff handoff = {.local_role = owner ? DVM_ORIGIN_PROVIDER : DVM_ORIGIN_TERMINAL};
    unsigned char owner_config[160]; /* Short trusted test launch configuration. */
    if (owner)
    {
        const char *count = getenv("DVM_TEST_SCALAR_CALL_COUNT");
        int size = snprintf((char *)owner_config, sizeof(owner_config),
            "implementation.dis\tcompute\t/scalar/request\t/scalar/reply\t4294967295\t64\t%s",
            count ? count : "1");
        if (size < 0 || (size_t)size >= sizeof(owner_config))
        {
            exit(2);
        }
        handoff.arguments = owner_config;
        handoff.description.argument_bytes = (size_t)size;
    }
    handoff.group = dvm_namespace_create(cwd, "/", NULL);
    strcpy(handoff.description.module, "scalar-guest.dis");
    if (!routes || !handoff.group || !dvm_namespace_pin(handoff.group) || dvm_stack_mount(handoff.group, routes, "/scalar", 0, 0) != 1)
    {
        fprintf(stderr, "scalar test could not mount route tree\n");
        exit(2);
    }
    dvm_provider_release(routes);
    if (!owner && !publish_adapter(handoff.group, original, profile, writer, reader))
    {
        fprintf(stderr, "scalar test could not bind trusted adapter\n");
        exit(2);
    }
    int result = dvm_runtime_main_with_handoff(argc, argv, &handoff);
    dvm_namespace_release(handoff.group);
    return result;
}


/* EN/RU: Mount both export routes in one owner namespace and run the shared service.
 * Смонтировать маршруты двух экспортов в одном пространстве владельца и запустить сервис. */
int scalar_runtime_shared(int argc, char **argv, DVM_Provider *const *requests, DVM_Provider *const *replies)
{
    char cwd[DVM_NS_PATH_LIMIT];
    const char *request_paths[] = {"/request0", "/request1"};
    const char *reply_paths[] = {"/reply0", "/reply1"};
    unsigned char arguments[] = "set\timplementation.dis\tadjust\t/scalar/request0\t/scalar/reply0"
        "\tcompute\t/scalar/request1\t/scalar/reply1";
    DVM_View *view = dvm_view_create(NULL, NULL, NULL);
    if (!view || !getcwd(cwd, sizeof(cwd)))
    {
        exit(2);
    }
    for (size_t i = 0; i < 2; i++)
    {
        if (!dvm_view_map(view, request_paths[i], requests[i], ".", FVM_ACCESS_READ, 0) ||
            !dvm_view_map(view, reply_paths[i], replies[i], ".", FVM_ACCESS_WRITE, 0))
        {
            exit(2);
        }
    }
    DVM_Provider *routes = dvm_view_freeze(view);
    DVM_LaunchHandoff handoff = {.local_role = DVM_ORIGIN_PROVIDER, .arguments = arguments};
    handoff.description.argument_bytes = sizeof(arguments) - 1;
    strcpy(handoff.description.module, "scalar-owner-set.dis");
    handoff.group = dvm_namespace_create(cwd, "/", NULL);
    if (!routes || !handoff.group || !dvm_namespace_pin(handoff.group) ||
        dvm_stack_mount(handoff.group, routes, "/scalar", 0, 0) != 1)
    {
        exit(2);
    }
    dvm_provider_release(routes);
    int result = dvm_runtime_main_with_handoff(argc, argv, &handoff);
    dvm_namespace_release(handoff.group);
    return result;
}


static pthread_mutex_t caller_tree_lock = PTHREAD_MUTEX_INITIALIZER; /* Fixture publication lock. */
static DVM_Provider *caller_tree; /* Borrowed through the fixture callback lease. */


/* EN: Publish the complete context for this one fixture call.
 * RU: Опубликовать полный контекст на время вызова стенда. */
static void *begin_caller(void *state, DVM_Provider *target, DVM_Provider *tree)
{
    (void)state;
    (void)target;
    pthread_mutex_lock(&caller_tree_lock);
    void *lease = NULL;
    if (!caller_tree)
    {
        caller_tree = tree;
        lease = tree;
    }
    pthread_mutex_unlock(&caller_tree_lock);
    return lease;
}


/* EN: Remove the borrowed tree after the matching exchange finishes.
 * RU: Снять заимствованное дерево по завершении соответствующего обмена. */
static void end_caller(void *state, void *lease)
{
    (void)state;
    pthread_mutex_lock(&caller_tree_lock);
    if (caller_tree == lease)
    {
        caller_tree = NULL;
    }
    pthread_mutex_unlock(&caller_tree_lock);
}


/* EN: Install the fixture's native binding before sharing the mount.
 * RU: Установить нативную связь стенда до публикации маунта. */
int scalar_guest_bind_caller(DVM_Provider *mount)
{
    DVM_CallerBinding binding = {.begin = begin_caller, .end = end_caller};
    return dvm_provider_bind_caller(mount, &binding);
}


/* EN: Check the received PID against the published .pid in the active context.
 * RU: Сверить полученный PID с опубликованным .pid активного контекста. */
int scalar_guest_check_caller(uint32_t pid)
{
    pthread_mutex_lock(&caller_tree_lock);
    DVM_Provider *tree = caller_tree;
    if (!dvm_provider_retain(tree))
    {
        tree = NULL;
    }
    pthread_mutex_unlock(&caller_tree_lock);
    DVM_Provider *program = dvm_program_capture(tree, pid);
    DVM_VfsHandle handle = {0};
    char expected[sizeof("4294967295\n")];
    char got[sizeof(expected)];
    size_t length = (size_t)snprintf(expected, sizeof(expected), "%" PRIu32 "\n", pid);
    int valid = program && dvm_provider_open(program, ".pid", "r", &handle) == FVM_OK;
    if (valid)
    {
        DVM_IoResult read = dvm_vfs_read_chunk(&handle, got, sizeof(got));
        valid = read.status == FVM_OK && read.count == length && !memcmp(got, expected, length);
    }
    dvm_vfs_close(&handle);
    dvm_provider_release(program);
    dvm_provider_release(tree);
    return valid;
}


/* EN: Verify a captured PID survives the module root release and keeps its
 * own read-only identity and explicit namespace grant.
 * RU: Проверить, что захваченный PID сохраняет свой идентификатор на чтение
 * и выданное пространство имён после освобождения корня модуля. */
static int verify_program_view(void)
{
    DVM_Provider *marker = dvm_module_adapter("test", 4);
    DVM_View *builder = dvm_view_create(NULL, NULL, NULL);
    if (!builder || !marker || !dvm_view_map(builder, "/env/CLIENT", marker, ".", FVM_ACCESS_READ, 0))
    {
        dvm_provider_release(marker);
        dvm_view_discard(builder);
        return 0;
    }
    dvm_provider_release(marker);
    DVM_Provider *vfs = dvm_view_freeze(builder);
    if (!vfs)
    {
        return 0;
    }
    DVM_ProgramView entries[] = {{.pid = 41}, {.pid = 42, .vfs = vfs}};
    DVM_ProgramPublication *control;
    DVM_Provider *root = dvm_program_publication(&control);
    int valid = root && dvm_program_publication_replace(control, entries, 2);
    dvm_provider_release(vfs);
    entries[1] = entries[0];
    valid = valid && !dvm_program_publication_replace(control, entries, 2);
    DVM_Provider *missing = dvm_program_capture(root, 43);
    DVM_Provider *program = dvm_program_capture(root, 42);
    valid = valid && program && !missing && dvm_program_publication_replace(control, entries, 1);
    dvm_provider_release(missing);
    DVM_Provider *retired = dvm_program_capture(root, 42);
    DVM_Provider *remaining = dvm_program_capture(root, 41);
    valid = valid && !retired && remaining && dvm_program_publication_replace(control, NULL, 0);
    dvm_provider_release(retired);
    dvm_provider_release(remaining);
    dvm_provider_release(root);
    if (!valid)
    {
        dvm_provider_release(program);
        return 0;
    }
    DVM_VfsHandle reader = {0};
    DVM_VfsHandle writer = {0};
    FVM_FileInfo info;
    char text[4] = {0};
    valid = dvm_provider_open(program, ".pid", "r", &reader) == FVM_OK;
    if (valid)
    {
        DVM_IoResult got = dvm_vfs_read_chunk(&reader, text, sizeof(text));
        valid = got.status == FVM_OK && got.count == 3 && !memcmp(text, "42\n", 3) &&
            dvm_provider_open(program, ".pid", "wo", &writer) != FVM_OK &&
            dvm_provider_info(program, ".vfs/env/CLIENT", &info) == FVM_OK &&
            dvm_provider_info(program, ".fd", &info) == FVM_NOT_FOUND;
    }
    dvm_vfs_close(&reader);
    dvm_vfs_close(&writer);
    dvm_provider_release(program);
    return valid;
}


/* EN/RU: Mount the borrowed remote root and run the guest with ordinary VFS lookup.
 * Смонтировать заимствованный корень и запустить гостя с обычным поиском VFS. */
int scalar_runtime_mounted(int argc, char **argv, DVM_Provider *mount)
{
    if (!verify_program_view())
    {
        fprintf(stderr, "could not verify the caller program tree\n");
        exit(2);
    }
    puts("PASS captured caller program tree");
    char cwd[DVM_NS_PATH_LIMIT];
    DVM_LaunchHandoff handoff = {.local_role = DVM_ORIGIN_TERMINAL};
    if (!getcwd(cwd, sizeof(cwd)))
    {
        exit(2);
    }
    handoff.group = dvm_namespace_create(cwd, "/", NULL);
    strcpy(handoff.description.module, "scalar-mounted.dis");
    if (!handoff.group || !dvm_namespace_pin(handoff.group) ||
        dvm_stack_mount(handoff.group, mount, "/peer", 0, 0) != 1)
    {
        fprintf(stderr, "could not install the plain Styx root at /peer\n");
        exit(2);
    }
    int result = dvm_runtime_main_with_handoff(argc, argv, &handoff);
    dvm_namespace_release(handoff.group);
    return result;
}
