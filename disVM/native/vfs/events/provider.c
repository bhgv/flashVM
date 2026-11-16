/* Copyright (c) 2026 bhgv
 * EN: Ordinary readonly VFS file projection of one explicitly granted native event inbox.
 * RU: Обычный readonly VFS file над одним явно выданным native event inbox.
 */
#include "internal.h"
#include <string.h>


/* EN/RU: Stable node identity and immutable readonly metadata; path/state are borrowed.
 * Stable node identity и immutable readonly metadata; path/state borrowed. */
static FVM_Result info(void *state, const char *path, FVM_FileInfo *out)
{
    StyxEvents *events = state;
    if (strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    memset(out, 0, sizeof(*out));
    strcpy(out->name, "events");
    out->mode = 0444;
    out->device.low = 0x45564e54UL;
    out->identity.low = events->identity & 0xffffffffULL;
    out->identity.high = events->identity >> 32;
    return FVM_OK;
}


/* EN/RU: Query retained file metadata without any provider/source I/O.
 * Получить metadata удержанного файла без provider/source I/O. */
static FVM_Result openinfo(void *state, FVM_FileInfo *out)
{
    return info(state, ".", out);
}


/* EN/RU: Read one record, preserving AGAIN/EOF distinction and SDK record boundary.
 * Прочитать одну запись, сохранив различие AGAIN/EOF и SDK record boundary. */
static void readchunk(void *state, void *bytes, size_t size, DVM_IoResult *out)
{
    memset(out, 0, sizeof(*out));
    out->flags = DVM_IO_PREFIX;
    out->status = FVM_OK;
    if (!size)
    {
        return;
    }
    int result = styxeventsread(state, bytes, size, &out->count);
    if (result == STYX_EVENT_RECORD)
    {
        out->flags |= DVM_IO_RECORD;
    }
    else if (result == STYX_EVENT_EOF)
    {
        out->stop = DVM_IO_EOF;
    }
    else
    {
        out->status = result == STYX_EVENT_SMALL ? FVM_INVALID_ARGUMENT : FVM_ERROR;
        out->stop = result == STYX_EVENT_EMPTY ? DVM_IO_AGAIN : DVM_IO_FAILURE;
    }
}


/* EN/RU: Consume this open's inbox owner ref, independent of the factory's own reference.
 * Поглотить owner ref открытия независимо от factory reference. */
static void closefile(void *state)
{
    styxeventsfree(state);
}
static const DVM_VfsOps opened = {openinfo, NULL, NULL, NULL, NULL, closefile, NULL, readchunk, NULL}; /* One shared stream. */


/* EN/RU: Open only this granted readonly node; provider wrapper supplies handle lifetime pins.
 * Открыть только выданную readonly node; provider wrapper удерживает lifetime handle. */
static FVM_Result openfile(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    if (strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    if (strcmp(mode, "r"))
    {
        return FVM_ERROR;
    }
    if (!styxeventsretain(state))
    {
        return FVM_OUT_OF_MEMORY;
    }
    out->ops = &opened;
    out->state = state;
    return FVM_OK;
}


/* EN/RU: Actual root access is readonly; caller's prepared namespace supplies additional confinement.
 * Root access только readonly; prepared namespace caller задаёт дополнительное confinement. */
static FVM_Result accessfile(void *state, const char *path, unsigned access)
{
    (void)state;
    if (strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    return access == FVM_ACCESS_READ ? FVM_OK : FVM_ERROR;
}
static const DVM_ProviderOps operations = {info, openfile, closefile, DVM_PROVIDER_STABLE_BIND, accessfile}; /* Stable single node. */


/* EN/RU: See events_provider.h; all consumers share the same stream head, codec and admission policy.
 * См. events_provider.h; consumers разделяют stream head, codec и admission policy. */
DVM_Provider *styxeventprovider(StyxEvents *events)
{
    DVM_Provider *provider = events ? dvm_provider_create(&operations, events) : NULL;
    if (!provider)
    {
        styxeventsfree(events);
    }
    return provider;
}
