/* Copyright (c) 2026 bhgv
 * EN: Open, retire and dispatch per-fid callback state in a process tree.
 * RU: Открытие, отзыв и обслуживание состояния колбэков fid в дереве процессов.
 */

typedef struct StyxRpcProgramOpen
{
    StyxRpcRegistration *entry;       /* Null after the application handle has closed. */
    void *cookie;                     /* Exact callback-owned handle. */
    struct StyxRpcProgramOpen *next;  /* Next open on the same registration. */
    struct StyxRpcProgramOpen **link; /* Address of the link pointing to this open. */
} OpenResource;


/* EN: Detach and close one handle before invoking the application close callback.
 * RU: Отсоединить и закрыть дескриптор до вызова колбэка приложения. */
static void retire_resource(OpenResource *file)
{
    StyxRpcRegistration *entry = file->entry;
    if (!entry)
    {
        return;
    }
    *file->link = file->next;
    if (file->next)
    {
        file->next->link = file->link;
    }
    file->entry = NULL;
    if (entry->resource.close)
    {
        entry->resource.close(file->cookie);
    }
}


#include "program_grant.inc"


/* EN/RU: See program.h. / См. program.h. */
int styxrpc_program_unibus_revoke(StyxRpcPrograms *programs, uint32_t pid)
{
    size_t slot = find_pid(programs, pid);
    if (!slot || !room(programs, 0))
    {
        return 0;
    }
    uint64_t parent = programs->nodes[slot].entry.id;
    for (size_t i = 1; i < programs->used; ++i)
    {
        StyxRpcRegistration *entry = &programs->nodes[i];
        if (entry->parent == parent && !entry->revoked && !strcmp(entry->name, ".unibus"))
        {
            if (parent != slot)
            {
                revoke_grant(programs, parent);
                return 1;
            }
            entry->revoked = 1;
            programs->revision++;
            while (entry->opens)
            {
                retire_resource(entry->opens);
            }
            return 1;
        }
    }
    return 0;
}


/* EN: Bind the current fid to registered data and its optional open callback.
 * RU: Закрепить данные регистрации и необязательный колбэк открытия за fid. */
static int open_resource(void *context, uint64_t id, unsigned access, void **output)
{
    const StyxRpcPrograms *programs = context;
    StyxRpcRegistration *entry = find_node(programs, id);
    if (!entry || entry->entry.directory)
    {
        return 0;
    }
    OpenResource *file = malloc(sizeof(*file));
    if (!file)
    {
        return 0;
    }
    *file = (OpenResource){.entry = entry, .cookie = entry->resource.context};
    if (entry->resource.open && entry->resource.open(entry->resource.context, access, &file->cookie) != 1)
    {
        free(file);
        return 0;
    }
    file->next = entry->opens;
    file->link = &entry->opens;
    if (file->next)
    {
        file->next->link = &file->next;
    }
    entry->opens = file;
    *output = file;
    return 1;
}


/* EN: Deliver a callback, one network-order integer or a PID byte slice.
 * RU: Вызвать колбэк чтения либо выдать int или часть текста PID. */
static int read_resource(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    OpenResource *file = cookie;
    const StyxRpcRegistration *entry = file->entry;
    if (!entry || entry->revoked)
    {
        return 0;
    }
    if (entry->pid)
    {
        char text[sizeof("4294967295\n")];
        size_t length = (size_t)snprintf(text, sizeof(text), "%" PRIu32 "\n", entry->pid);
        size_t left = offset < length ? length - (size_t)offset : 0;
        *size = left < capacity ? left : capacity;
        if (*size)
        {
            memcpy(bytes, text + offset, *size);
        }
        return 1;
    }
    if (entry->variable.load)
    {
        *size = capacity;
        return styxintvariableread(&entry->variable, offset, bytes, size);
    }
    return entry->resource.read && entry->resource.read(file->cookie, offset, bytes, capacity, size);
}


/* EN: Deliver exactly one callback or registered signed store.
 * RU: Передать одну запись колбэку либо зарегистрированной переменной. */
static int write_resource(void *cookie, uint64_t offset, const void *bytes, size_t size, size_t *written)
{
    OpenResource *file = cookie;
    const StyxRpcRegistration *entry = file->entry;
    if (!entry || entry->revoked)
    {
        return 0;
    }
    if (!entry->variable.store)
    {
        return entry->resource.write && entry->resource.write(file->cookie, offset, bytes, size, written);
    }
    if (!styxintvariablewrite(&entry->variable, offset, bytes, size))
    {
        return 0;
    }
    *written = size;
    return 1;
}


/* EN: Retire the exact callback state, including on disconnection.
 * RU: Закрыть точное состояние колбэка, в том числе при обрыве. */
static void close_resource(void *cookie)
{
    OpenResource *file = cookie;
    retire_resource(file);
    free(file);
}
