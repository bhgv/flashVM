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
#include "call_scope.h"
#include "flashvm/srecord.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>


/* EN/RU: See unibus_server_state.h. / См. unibus_server_state.h. */
int styxunibusprepareload(MountedExport *module, const char *name)
{
    if (!styxscalar_name_length(name))
    {
        return 0;
    }
    strcpy(module->load_name, name);
    module->load_offer = styxunibusprofilemailbox();
    module->owner.request = styxunibusloadmailbox();
    module->owner.reply = styxunibusloadmailbox();
    return module->load_offer && module->owner.request && module->owner.reply;
}


/* EN: Wait at most three seconds for a complete local offer. Timeout leaves
 * the unopened interface eligible for a later attempt.
 * RU: Ждать полное локальное предложение до трёх секунд. При истечении срока
 * разрешить новую попытку открытия интерфейса. */
static size_t read_offer(DVM_Provider *offer, void *bytes, size_t capacity, const struct timespec *start)
{
    DVM_VfsHandle reader = {0};
    struct timespec clock;
    if (dvm_provider_open(offer, ".", "r", &reader) != FVM_OK)
    {
        return 0;
    }
    size_t size = 0;
    while (!clock_gettime(CLOCK_MONOTONIC, &clock) &&
        (clock.tv_sec - start->tv_sec) * INT64_C(1000) + (clock.tv_nsec - start->tv_nsec) / 1000000 < 3000)
    {
        const DVM_IoResult got = dvm_vfs_read_chunk(&reader, bytes, capacity);
        if (got.status == FVM_OK && (got.flags & DVM_IO_RECORD) && !(got.flags & DVM_IO_UNCERTAIN) &&
            got.count <= capacity)
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
    if (!server->loaded || server->load_state)
    {
        return !server->loaded || server->load_state == 1;
    }
    struct timespec start;
    if (clock_gettime(CLOCK_MONOTONIC, &start))
    {
        return 0;
    }
    MountedExport *directory = &server->modules[0];
    while (server->load_ready < server->count)
    {
        MountedExport *module = &server->modules[server->load_ready];
        unsigned char offer[STYX_SHAPED_OFFER_MAX];
        size_t size = read_offer(module->load_offer, offer, sizeof(offer), &start);
        if (!size)
        {
            return 0;
        }
        styxunibusmailboxseal(module->load_offer);
        unsigned long tag = styxloadedacceptshape(module->load_name, offer, size, &module->owner.shape);
        if (server->caller_root && (tag == STYX_DYNAMIC_REQUEST_TAG ||
            tag == STYX_REF_REQUEST_TAG || tag == STYX_REF_PAIR_REQUEST_TAG) && module->owner.shape.arguments[0])
        {
            FVM_SRecord profile;
            size_t used;
            uint32_t signature;
            if (fvm_srecord_read(offer, size, sizeof(offer), &profile, &used) != FVM_S_OK ||
                !styxshapedpayload(profile.payload, profile.size, NULL, &signature, NULL))
            {
                tag = 0;
            }
            else if (!styxloadedrequest(signature))
            {
                if (fvm_srecord_write(STYX_SCOPED_OFFER_TAG, profile.payload, profile.size,
                    directory->interface + directory->interface_size, sizeof(directory->interface) -
                        directory->interface_size, &used) != FVM_S_OK)
                {
                    tag = 0;
                }
                else
                {
                    memcpy(offer, directory->interface + directory->interface_size, used);
                    size = used;
                    module->function.program_root = server->caller_root;
                }
            }
        }
        if (!tag || !styxscalarownerbind(&module->owner, &module->function, tag))
        {
            server->load_state = -1;
            for (size_t i = 0; i < server->count; i++)
            {
                styxunibusmailboxseal(server->modules[i].owner.request);
            }
            return 0;
        }
        memcpy(directory->interface + directory->interface_size, offer, size);
        directory->interface_size += size;
        server->load_ready++;
    }
    directory->profile_node->d.length = directory->interface_size;
    server->load_state = 1;
    return 1;
}


/* EN/RU: See unibus_server.h. / См. unibus_server.h. */
DVM_Provider *styxunibusserverloadprofile(StyxUnibusServer *server)
{
    return styxunibusserverloadprofileat(server, 0);
}



/* EN/RU: See unibus_server.h. / См. unibus_server.h. */
StyxUnibusServer *styxunibusserverstartmounted(RemotePhy *channel,
    const StyxMountedScalar *entries, size_t count)
{
    return styxunibusstarttable(channel, entries, count, 0, NULL);
}


/* EN/RU: See unibus_server.h. / См. unibus_server.h. */
StyxUnibusServer *styxunibusserverstartloaded(RemotePhy *channel, const char *module, const char *name)
{
    return styxunibusserverstartloadedset(channel, module, &name, 1);
}


/* EN: Reserve loaded owner endpoints with an optional native reverse root.
 * RU: Зарезервировать экспорты владельца с необязательным встречным корнем. */
static StyxUnibusServer *start_loaded_set(RemotePhy *channel, const char *module,
    const char *const *names, size_t count, DVM_Provider *reverse)
{
    if (!names || !count || count > STYX_MOUNTED_EXPORT_LIMIT)
    {
        phyfree(channel);
        return NULL;
    }
    StyxMountedScalar entries[STYX_MOUNTED_EXPORT_LIMIT] = {0};
    for (size_t i = 0; i < count; i++)
    {
        entries[i].module = module;
        entries[i].name = names[i];
    }
    return styxunibusstarttable(channel, entries, count, 1, reverse);
}


/* EN/RU: See unibus_server.h. / См. unibus_server.h. */
StyxUnibusServer *styxunibusserverstartloadedset(RemotePhy *channel, const char *module,
    const char *const *names, size_t count)
{
    return start_loaded_set(channel, module, names, count, NULL);
}


/* EN/RU: See unibus_server.h. / См. unibus_server.h. */
StyxUnibusServer *styxunibusserverstartscopedset(RemotePhy *channel, const char *module,
    const char *const *names, size_t count, DVM_Provider *reverse)
{
    if (!reverse)
    {
        phyfree(channel);
        return NULL;
    }
    return start_loaded_set(channel, module, names, count, reverse);
}


/* EN/RU: See unibus_server.h. / См. unibus_server.h. */
DVM_Provider *styxunibusserverloadprofileat(StyxUnibusServer *server, size_t index)
{
    return server && index < server->count ? server->modules[index].load_offer : NULL;
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
