/* Copyright (c) 2026 bhgv
 * EN: Mount the two live Styx file grants before the real guest starts.
 * RU: Смонтировать два живых файла Styx до старта настоящего гостя.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lib9.h"
#include "dat.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "launch_files.h"
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
            getenv("DVM_TEST_SCALAR_TUPLE_OWNER") ?
                "tuple\ttuple-owner.dis\ttransform\t/scalar/request\t/scalar/reply" :
            getenv("DVM_TEST_SCALAR_BIG_OWNER") ?
                "big\twide-owner.dis\techo\t/scalar/request\t/scalar/reply\t76\t73" :
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
    const char *arguments = getenv("DVM_TEST_SCALAR_MIXED_OWNER") ?
        "mixed\twide-owner.dis\ti:count\t/scalar/request0\t/scalar/reply0"
        "\tb:echo\t/scalar/request1\t/scalar/reply1" :
        "set\timplementation.dis\tadjust\t/scalar/request0\t/scalar/reply0"
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
    DVM_LaunchHandoff handoff = {.local_role = DVM_ORIGIN_PROVIDER, .arguments = (unsigned char *)arguments};
    handoff.description.argument_bytes = strlen(arguments);
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


/* EN: Build a launch grant that imports FD 3 and labels the initial program.
 * RU: Подготовить разрешение запуска с импортом FD 3 и меткой программы. */
static DVM_Provider *fixture_program(const char *bytes)
{
    DVM_Provider *file = dvm_module_adapter(bytes, strlen(bytes));
    DVM_Provider *identity = dvm_module_adapter("41\n", 3);
    DVM_View *builder = dvm_view_create(NULL, NULL, NULL);
    int mapped = file && identity && builder &&
        dvm_view_map(builder, "/.fd/3", file, ".", FVM_ACCESS_READ, 0) &&
        dvm_view_map(builder, "/.pid", identity, ".", FVM_ACCESS_READ, 0);
    dvm_provider_release(file);
    dvm_provider_release(identity);
    if (!mapped)
    {
        dvm_view_discard(builder);
        return NULL;
    }
    return dvm_view_freeze(builder);
}


/* EN: Verify two aliases retain one imported slot and advance the same cursor.
 * RU: Проверить удержание одного слота двумя псевдонимами и продвижение общей позиции. */
static int verify_imported_fds(void)
{
    DVM_LaunchHandoff handoff = {.program = fixture_program("leftnext")};
    DVM_LaunchFiles *files = NULL;
    int valid = handoff.program && dvm_launch_files_open(&handoff, &files);
    dvm_provider_release(handoff.program);
    DVM_Provider *directory = valid ? dvm_launch_fd_view(files, &valid) : NULL;
    dvm_launch_files_release(files);
    DVM_VfsHandle first = {0}, second = {0}, denied = {0};
    valid = valid && directory &&
        dvm_provider_open(directory, "3", "r", &first) == FVM_OK &&
        dvm_provider_open(directory, "3", "r", &second) == FVM_OK &&
        dvm_provider_open(directory, "3", "wo", &denied) != FVM_OK;
    dvm_provider_release(directory);
    if (valid)
    {
        char left[4], next[4];
        DVM_IoResult a = dvm_vfs_read_chunk(&first, left, sizeof(left));
        DVM_IoResult b = dvm_vfs_read_chunk(&second, next, sizeof(next));
        valid = a.status == FVM_OK && b.status == FVM_OK && a.count == 4 && b.count == 4 &&
            !memcmp(left, "left", 4) && !memcmp(next, "next", 4);
    }
    dvm_vfs_close(&first);
    dvm_vfs_close(&second);
    dvm_vfs_close(&denied);
    return valid;
}


/* EN: Mount one granted fixture subtree in the guest's namespace.
 * RU: Смонтировать одно выданное поддерево в пространстве имён гостя. */
static int mount_context(DVM_Namespace *group, const char *at, const char *leaf)
{
    DVM_View *view = dvm_view_create(NULL, NULL, NULL);
    DVM_Provider *file = dvm_module_adapter(at, strlen(at));
    int mapped = view && file && dvm_view_map(view, leaf, file, ".", FVM_ACCESS_READ, 0);
    dvm_provider_release(file);
    if (!mapped)
    {
        dvm_view_discard(view);
        return 0;
    }
    DVM_Provider *tree = dvm_view_freeze(view);
    int mounted = tree && dvm_stack_mount(group, tree, at, 0, 0) == 1;
    dvm_provider_release(tree);
    return mounted;
}


extern int scalar_verify_host_stdin(void); /* Regular stdin alias check. */
extern int scalar_verify_host_socket(void); /* Socket stdin alias check. */


/* EN/RU: Mount the borrowed remote root and run the guest with ordinary VFS lookup.
 * Смонтировать заимствованный корень и запустить гостя с обычным поиском VFS. */
int scalar_runtime_mounted(int argc, char **argv, DVM_Provider *mount)
{
    if (getenv("DVM_TEST_SCALAR_HOST_STDIN"))
    {
        if (!scalar_verify_host_stdin())
        {
            fprintf(stderr, "could not verify redirected host stdin\n");
            exit(2);
        }
        puts("PASS redirected host stdin shares its file position");
    }
    if (getenv("DVM_TEST_SCALAR_HOST_SOCKET"))
    {
        if (!scalar_verify_host_socket())
        {
            fprintf(stderr, "could not verify socket host stdin\n");
            exit(2);
        }
        puts("PASS socket stdio preserves blocking flags");
    }
    if (!verify_program_view() || !verify_imported_fds())
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
    int scoped = getenv("DVM_TEST_SCALAR_CALLER") != NULL;
    handoff.program = scoped ? fixture_program("leftnextlast") : NULL;
    strcpy(handoff.description.module, "scalar-mounted.dis");
    if (!handoff.group || !dvm_namespace_pin(handoff.group) ||
        dvm_stack_mount(handoff.group, mount, "/peer", 0, 0) != 1 ||
        (scoped &&
            (!handoff.program ||
             !mount_context(handoff.group, "/.vfs", "/env/CLIENT") ||
             !mount_context(handoff.group, "/.var", "/counter"))))
    {
        fprintf(stderr, "could not install the plain Styx root at /peer\n");
        exit(2);
    }
    int result = dvm_runtime_main_with_handoff(argc, argv, &handoff);
    dvm_namespace_release(handoff.group);
    dvm_provider_release(handoff.program);
    if (scoped && result == 0)
    {
        puts("PASS typed Styx caller PID");
    }
    return result;
}
