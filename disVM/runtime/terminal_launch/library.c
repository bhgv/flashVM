/* Copyright (c) 2026 bhgv
 * EN: Publish a host-configured Dis export table over one delegated Styx stream.
 * RU: Опубликовать заданную хостом таблицу экспортов Dis через один поток Styx.
 */
#include "config.h"
#include "unibus_server.h"
#include "mount.h"
#include "offer.h"
#include "vfs/module_adapter.h"
#include "vfs/export.h"
#include "vfs/launch/runtime.h"
#include "drivers/posix/fd.h"
#include "mux/bidir.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define LIBRARY_CONFIG_CAPACITY 4097 /* Dispatcher startup limit plus NUL. */


extern DVM_Provider *dvm_terminal_publish_program(DVM_Namespace *, const char *);
extern int dvm_terminal_prepare_shared_launch(StyxUnibusServer *, DVM_Provider *, DVM_LaunchHandoff *, unsigned char *);

static StyxUnibusServer *launch_server; /* Borrowed from the active CLI through runtime shutdown. */
static DVM_Provider *launch_root; /* Borrowed reverse session. */
static const char *launch_image; /* Borrowed host-selected image. */
static int launch_taken; /* One use by the dispatcher. */


/* EN: Publish a hosted child result without ending the library session.
 * RU: Опубликовать результат потомка и сохранить сессию библиотек. */
static void report_shared_launch(void *server, int status)
{
    styxunibusserverlaunchstatus(server, status);
}


/* EN: Wait for the configured request outside managed admission, at most once.
 * RU: Дождаться настроенного запроса вне допуска VM, не более одного раза. */
int dvm_terminal_take_shared_launch(DVM_LaunchHandoff *handoff, unsigned char *arguments,
    const char **image, DVM_LaunchCompletion *completion)
{
    if (!launch_server || __atomic_exchange_n(&launch_taken, 1, __ATOMIC_ACQ_REL))
    {
        return 0;
    }
    handoff->local_role = DVM_ORIGIN_PROVIDER;
    *completion = (DVM_LaunchCompletion){report_shared_launch, launch_server};
    *image = launch_image;
    if (!dvm_terminal_prepare_shared_launch(launch_server, launch_root, handoff, arguments))
    {
        report_shared_launch(launch_server, 1);
        return 0;
    }
    strcpy(handoff->description.module, DVM_STYX_LAUNCH_ENTRY);
    return 1;
}


/* EN: Build a private mailbox tree and matching startup text. Return an owned
 * provider or NULL. The server owns the borrowed inboxes through join.
 * RU: Построить частное дерево приёмников и стартовую строку. Вернуть свою
 * ссылку или NULL. Приёмники принадлежат серверу до join. */
static DVM_Provider *library_routes(StyxUnibusServer *server, const StyxMountedScalar *entries,
    const char *const *images, size_t count, int execute, char config[LIBRARY_CONFIG_CAPACITY])
{
    DVM_View *view = dvm_view_create(NULL, NULL, NULL);
    if (!view)
    {
        return NULL;
    }
    strcpy(config, execute ? "loaded-launch-table" : "loaded-table");
    size_t used = strlen(config);
    for (size_t i = 0; i < count; i++)
    {
        char request[24], reply[24], offer[24];
        snprintf(request, sizeof(request), "/request%zu", i);
        snprintf(reply, sizeof(reply), "/reply%zu", i);
        snprintf(offer, sizeof(offer), "/offer%zu", i);
        int added = snprintf(config + used, LIBRARY_CONFIG_CAPACITY - used,
            "\t%s\t%s\t/.scalar%s\t/.scalar%s\t/.scalar%s", images[i], entries[i].name, request, reply, offer);
        if (added < 0 || (size_t)added >= LIBRARY_CONFIG_CAPACITY - used ||
            !dvm_view_map(view, request, styxunibusservermailbox(server, i, 0), ".", FVM_ACCESS_READ, 0) ||
            !dvm_view_map(view, reply, styxunibusservermailbox(server, i, 1), ".", FVM_ACCESS_WRITE, 0) ||
            !dvm_view_map(view, offer, styxunibusserverloadprofileat(server, i), ".", FVM_ACCESS_WRITE, 0))
        {
            dvm_view_discard(view);
            return NULL;
        }
        used += (size_t)added;
    }
    return dvm_view_freeze(view);
}


/* EN: Consume the local launch configuration and run until the peer disconnects.
 * RU: Принять локальную конфигурацию и работать до отсоединения клиента. */
int dvm_styx_library_cli(int argc, char **argv)
{
    StyxMountedScalar entries[STYX_MOUNTED_SCALAR_LIMIT] = {0};
    const char *images[STYX_MOUNTED_SCALAR_LIMIT];
    size_t count = 0;
    int separator = 3;
    const char *execute = NULL;
    if (argc > 4 && !strcmp(argv[3], "--execute"))
    {
        execute = argv[4];
        separator += 2;
    }
    for (; argc - separator >= 4 && count < STYX_MOUNTED_SCALAR_LIMIT &&
        !strcmp(argv[separator], "--export"); separator += 4)
    {
        entries[count].module = argv[separator + 1];
        images[count] = argv[separator + 2];
        entries[count].name = argv[separator + 3];
        if (!*images[count] || strchr(images[count], '\t') || strchr(entries[count].name, '\t'))
        {
            break;
        }
        count++;
    }
    if (!count || argc <= separator + 1 || strcmp(argv[separator], "--"))
    {
        fprintf(stderr, "usage: disvm --styx-library-serve-fd FD [--execute PROGRAM.dis] --export PUBLIC.dis OWNER.dis NAME [...] -- [VM OPTIONS] scalarowner.dis\n");
        return 2;
    }
    int fd = dvm_terminal_connected(argv[2]);
    int breaker = fd >= 0 ? dup(fd) : -1;
    if (breaker < 0)
    {
        if (fd >= 0)
        {
            close(fd);
        }
        fprintf(stderr, "disvm: expected a delegated connected stream FD >= 3\n");
        return 2;
    }
    PhyBidir *mux = phybidircreate(phyposixfd(fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), 65536);
    StyxClient *reverse = mux ? styxclientmountphy(phybidirchannel(mux, 1), 4096, "provider", "", 5000, 8) : NULL;
    DVM_Provider *root = reverse ? styxmountprovider(reverse, FVM_ACCESS_MASK) : NULL;
    char cwd[DVM_NS_PATH_LIMIT], config[LIBRARY_CONFIG_CAPACITY];
    int have_cwd = getcwd(cwd, sizeof(cwd)) != NULL;
    DVM_Namespace *host = execute && have_cwd ? dvm_namespace_create("/", cwd, NULL) : NULL;
    DVM_Provider *original = host && dvm_namespace_pin(host) ? dvm_terminal_publish_program(host, execute) : NULL;
    DVM_Provider *profile = original ? dvm_module_adapter(DVM_STYX_LAUNCH_CONTEXT, sizeof(DVM_STYX_LAUNCH_CONTEXT) - 1) : NULL;
    const StyxUnibusLaunch launch = {DVM_STYX_LAUNCH_TAG, original, profile};
    StyxUnibusServer *server = NULL;
    if (root && (!execute || profile))
    {
        server = execute ? styxunibusserverstartscopedlaunchtable(phybidirchannel(mux, 0), entries, count, root, &launch) :
            styxunibusserverstartscopedtable(phybidirchannel(mux, 0), entries, count, root);
    }
    dvm_provider_release(profile);
    dvm_provider_release(original);
    dvm_namespace_release(host);
    DVM_Provider *routes = server ? library_routes(server, entries, images, count, execute != NULL, config) : NULL;
    DVM_LaunchHandoff handoff = {.local_role = DVM_ORIGIN_PROVIDER};
    handoff.group = routes && have_cwd ? dvm_namespace_create(cwd, "/", NULL) : NULL;
    DVM_NamespacePlan target;
    int initialized = handoff.group && dvm_namespace_pin(handoff.group) &&
        dvm_namespace_plan(handoff.group, "/.scalar", &target) &&
        dvm_namespace_mount_provider(handoff.group, &target, routes, "/styx-library-routes") == 1;
    int result = 1;
    if (initialized)
    {
        strcpy(handoff.description.module, "styx-library-owner.dis");
        handoff.arguments = (unsigned char *)config;
        handoff.description.argument_bytes = strlen(config);
        launch_server = execute ? server : NULL;
        launch_root = root;
        launch_image = execute;
        result = dvm_runtime_main_with_handoff(argc - separator, argv + separator, &handoff);
    }
    else
    {
        fprintf(stderr, "disvm: cannot initialize the scoped Styx library server\n");
    }
    shutdown(breaker, SHUT_RDWR);
    close(breaker);
    styxunibusserverjoin(server);
    dvm_namespace_release(handoff.group);
    dvm_provider_release(routes);
    dvm_provider_release(root);
    phybidirfree(mux);
    return result;
}
