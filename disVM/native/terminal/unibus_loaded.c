/* Copyright (c) 2026 bhgv
 * EN: Bind a mounted Styx endpoint to the owner VM's actual load result.
 * RU: Связывание конечной точки Styx с результатом load у владельца.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "unibus_server.h"
#include "scalar_function.h"
#include "scalar_function_owner.h"
#include "scalar_verify.h"
#include "load_profile.h"
#include "file_provider.h"
#include "unibus_server_state.h"
#include <string.h>
#include <time.h>


/* EN/RU: See unibus_server_state.h. / См. unibus_server_state.h. */
int styxunibusprepareload(StyxUnibusServer *server, MountedExport *module, const char *name)
{
    if (!styxscalar_name_length(name))
    {
        return 0;
    }
    strcpy(server->load_name, name);
    server->load_offer = styxunibusmailbox(STYX_FUNCTION_PROFILE_TAG);
    module->owner.request = styxunibusloadmailbox();
    module->owner.reply = styxunibusloadmailbox();
    return server->load_offer && module->owner.request && module->owner.reply;
}


/* EN: Wait at most three seconds for a complete local offer. Timeout leaves
 * the unopened interface eligible for a later attempt.
 * RU: Ждать полное локальное предложение до трёх секунд. При истечении срока
 * разрешить новую попытку открытия интерфейса. */
static size_t read_offer(DVM_Provider *offer, void *bytes, size_t capacity)
{
    DVM_VfsHandle reader = {0};
    struct timespec start, clock;
    if (clock_gettime(CLOCK_MONOTONIC, &start) || dvm_provider_open(offer, ".", "r", &reader) != FVM_OK)
    {
        return 0;
    }
    size_t size = 0;
    while (!clock_gettime(CLOCK_MONOTONIC, &clock) &&
        (clock.tv_sec - start.tv_sec) * INT64_C(1000) + (clock.tv_nsec - start.tv_nsec) / 1000000 < 3000)
    {
        const DVM_IoResult got = dvm_vfs_read_chunk(&reader, bytes, capacity);
        if (got.status == FVM_OK && (got.flags & DVM_IO_RECORD) && !(got.flags & DVM_IO_UNCERTAIN))
        {
            size = got.count;
            break;
        }
        if (got.stop != DVM_IO_AGAIN || got.count || (got.flags & DVM_IO_UNCERTAIN))
        {
            break;
        }
        const struct timespec delay = {0, 10000000};
        nanosleep(&delay, NULL);
    }
    dvm_vfs_close(&reader);
    return size;
}


/* EN/RU: See unibus_server_state.h. / См. unibus_server_state.h. */
int styxunibusawaitload(StyxUnibusServer *server)
{
    if (!server->load_offer || server->load_state)
    {
        return !server->load_offer || server->load_state == 1;
    }
    MountedExport *module = &server->modules[0];
    size_t size = read_offer(server->load_offer, module->interface, sizeof(module->interface));
    if (!size)
    {
        return 0;
    }
    styxunibusmailboxseal(server->load_offer);
    unsigned long tag = styxloadedaccept(server->load_name, module->interface, size);
    if (!tag || !styxscalarownerbind(&module->owner, &module->function, tag))
    {
        server->load_state = -1;
        return 0;
    }
    module->interface_size = size;
    module->profile_node->d.length = size;
    server->load_state = 1;
    return 1;
}


/* EN/RU: See unibus_server.h. / См. unibus_server.h. */
DVM_Provider *styxunibusserverloadprofile(StyxUnibusServer *server)
{
    return server ? server->load_offer : NULL;
}



/* EN/RU: See unibus_server.h. / См. unibus_server.h. */
StyxUnibusServer *styxunibusserverstartmounted(RemotePhy *channel,
    const StyxMountedScalar *entries, size_t count)
{
    return styxunibusstarttable(channel, entries, count, 0);
}


/* EN/RU: See unibus_server.h. / См. unibus_server.h. */
StyxUnibusServer *styxunibusserverstartloaded(RemotePhy *channel, const char *module, const char *name)
{
    const StyxMountedScalar entry = {.module = module, .name = name};
    return styxunibusstarttable(channel, &entry, 1, 1);
}


/* EN/RU: Publish the copied native function on a consumed channel.
 * Опубликовать скопированную нативную функцию на принятом канале. */
StyxUnibusServer *styxunibusserverstartfunction(RemotePhy *channel, const char *module,
    const char *name, const StyxScalarFunction *function)
{
    if (!function)
    {
        phyfree(channel);
        return NULL;
    }
    const StyxMountedScalar entry = {module, name, function, 0, 0, 0, 0, 0};
    return styxunibusserverstartmounted(channel, &entry, 1);
}


/* EN/RU: Publish one named export backed by the shared Dis service.
 * Опубликовать один именованный экспорт через общий сервис Dis. */
StyxUnibusServer *styxunibusserverstartdis(RemotePhy *channel, const char *module, const char *name)
{
    const StyxMountedScalar entry = {module, name, NULL, 0, 0, 0, 0, 0};
    return styxunibusserverstartmounted(channel, &entry, 1);
}
