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
    module->owner.request = styxunibusmanagedmailbox();
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
    while (server->load_ready < server->count)
    {
        MountedExport *module = &server->modules[server->load_ready];
        MountedExport *directory = &server->modules[module->directory_leader];
        unsigned char offer[STYX_SHAPED_OFFER_MAX];
        size_t size = read_offer(module->load_offer, offer, sizeof(offer), &start);
        if (!size)
        {
            return 0;
        }
        styxunibusmailboxseal(module->load_offer);
        unsigned long tag = styxloadedacceptshape(module->load_name, offer, size, &module->owner.shape);
        if (server->caller_root && (tag == STYX_DYNAMIC_REQUEST_TAG || tag == STYX_MANAGED_REQUEST_TAG ||
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
                if (fvm_srecord_write(tag == STYX_MANAGED_REQUEST_TAG ? STYX_SCOPED_RECORD_OFFER_TAG :
                    STYX_SCOPED_PATH_OFFER_TAG, profile.payload, profile.size,
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
        if (size > sizeof(directory->interface) - directory->interface_size || !tag || !styxscalarownerbind(&module->owner, &module->function, tag))
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
        directory->profile_node->d.length = directory->interface_size;
        server->load_ready++;
    }
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
    return styxunibusstarttable(channel, entries, count, 0, NULL, NULL);
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
    return styxunibusstarttable(channel, entries, count, 1, reverse, NULL);
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
StyxUnibusServer *styxunibusserverstartscopedtable(RemotePhy *channel,
    const StyxMountedScalar *entries, size_t count, DVM_Provider *reverse)
{
    if (!reverse)
    {
        phyfree(channel);
        return NULL;
    }
    return styxunibusstarttable(channel, entries, count, 1, reverse, NULL);
}


/* EN/RU: See unibus_server.h. / См. unibus_server.h. */
StyxUnibusServer *styxunibusserverstartlaunchmounted(RemotePhy *channel,
    const StyxMountedScalar *entries, size_t count, const StyxUnibusLaunch *launch)
{
    if (!launch)
    {
        phyfree(channel);
        return NULL;
    }
    return styxunibusstarttable(channel, entries, count, 0, NULL, launch);
}


/* EN/RU: See unibus_server.h. / См. unibus_server.h. */
StyxUnibusServer *styxunibusserverstartscopedlaunchtable(RemotePhy *channel,
    const StyxMountedScalar *entries, size_t count, DVM_Provider *reverse, const StyxUnibusLaunch *launch)
{
    if (!reverse || !launch)
    {
        phyfree(channel);
        return NULL;
    }
    return styxunibusstarttable(channel, entries, count, 1, reverse, launch);
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


/* EN/RU: See unibus_server.h. / См. unibus_server.h. */
void styxunibusserverlaunchstatus(StyxUnibusServer *server, int status)
{
    int pending = 0;
    __atomic_compare_exchange_n(&server->exit_value, &pending, status ? 2 : 1, 0,
        __ATOMIC_RELEASE, __ATOMIC_RELAXED);
}


/* EN/RU: See unibus_server.h. / См. unibus_server.h. */
void styxunibusserverexit(StyxUnibusServer *server, int status)
{
    styxunibusserverlaunchstatus(server, status);
    const struct timespec pause = {0, 10000000};
    for (unsigned attempt = 0; attempt < 500 && !__atomic_load_n(&server->stopped, __ATOMIC_ACQUIRE); attempt++)
    {
        nanosleep(&pause, NULL);
    }
}

/* EN/RU: See unibus_server.h. / См. unibus_server.h. */
StyxUnibusServer *styxunibusserverstartexit(RemotePhy *channel, unsigned long tag,
    DVM_Provider *original, DVM_Provider *profile)
{
    return styxunibusstart(channel, tag, original, profile, 0, 1);
}


/* EN: Read the immutable result, or an empty prefix while pending.
 * RU: Прочитать неизменяемый результат или пустой префикс в ожидании. */
size_t styxunibusexitread(StyxUnibusServer *server, char *bytes, size_t count, uint64_t offset)
{
    int value = __atomic_load_n(&server->exit_value, __ATOMIC_ACQUIRE);
    const char *status = value == 1 ? "ok\n" : value == 2 ? "error\n" : "";
    size_t size = strlen(status);
    if (offset >= size)
    {
        return 0;
    }
    size_t available = size - (size_t)offset;
    count = count < available ? count : available;
    memcpy(bytes, status + offset, count);
    return count;
}


/* EN/RU: See unibus_server_state.h. / См. unibus_server_state.h. */
void styxunibusdrainfunctions(StyxUnibusServer *server)
{
    const struct timespec pause = {0, 1000000};
    while (__atomic_load_n(&server->function_workers, __ATOMIC_ACQUIRE))
    {
        nanosleep(&pause, NULL);
    }
}
