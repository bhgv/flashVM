/* Copyright (c) 2026 bhgv
 * EN: Stable Qids and atomic, bounded registration for reverse process contexts.
 * RU: Устойчивые Qid и атомарная ограниченная регистрация встречных контекстов.
 */
#include "collection_internal.h"
#include "bigtable_internal.h"
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#define PROCESS_NODES 5U /* One process directory and four standard children. */
#define FD_CHILD 2U      /* .fd offset from the process directory. */
#define VAR_CHILD 3U     /* .var offset from the process directory. */
#define VFS_CHILD 4U     /* .vfs offset from the process directory. */


/* EN: Append one checked component to reserved stable storage.
 * RU: Добавить проверенный компонент в зарезервированную устойчивую память. */
static StyxRpcRegistration *append(StyxRpcPrograms *programs, uint64_t parent, const char *name,
    unsigned mode, int directory)
{
    size_t slot = programs->used++;
    StyxRpcRegistration *entry = &programs->nodes[slot];
    *entry = (StyxRpcRegistration){0};
    strcpy(entry->name, name);
    entry->entry = (StyxRpcEntry){slot, entry->name, mode, directory};
    entry->parent = parent;
    return entry;
}


/* EN: Find one registered process directory, returning zero if absent.
 * RU: Найти каталог зарегистрированного процесса, ноль при отсутствии. */
static size_t find_process(const StyxRpcPrograms *programs, uint32_t pid)
{
    if (programs && programs->nodes && pid)
    {
        for (size_t i = 2; i < programs->used; ++i)
        {
            if (programs->nodes[i].pid == pid && programs->nodes[i].entry.directory)
            {
                return i;
            }
        }
    }
    return 0;
}


/* EN: Check bounded caller storage before any registration writes.
 * RU: Проверить границы памяти приложения до записи регистрации. */
static int room(const StyxRpcPrograms *programs, size_t slots)
{
    return programs && programs->nodes && programs->used <= programs->capacity &&
        slots <= programs->capacity - programs->used && programs->revision != UINT32_MAX;
}


/* EN/RU: See program.h. / См. program.h. */
int styxrpc_programs_init(StyxRpcPrograms *programs, StyxRpcRegistration *storage, size_t capacity)
{
    if (!programs || !storage || capacity < 2 || capacity > STYXRPC_TREE_CHILDREN)
    {
        return 0;
    }
    *programs = (StyxRpcPrograms){.nodes = storage, .capacity = capacity};
    append(programs, 0, "/", 0555, 1);
    append(programs, 0, ".prog", 0555, 1);
    return 1;
}


/* EN/RU: See program.h. / См. program.h. */
int styxrpc_program_add(StyxRpcPrograms *programs, uint32_t pid)
{
    if (!room(programs, PROCESS_NODES) || !pid || find_process(programs, pid))
    {
        return 0;
    }
    char text[sizeof("4294967295")];
    snprintf(text, sizeof(text), "%" PRIu32, pid);
    size_t slot = programs->used;
    append(programs, 1, text, 0555, 1)->pid = pid;
    StyxRpcRegistration *identity = append(programs, slot, ".pid", 0444, 0);
    identity->pid = pid;
    identity->length = strlen(text) + 1;
    append(programs, slot, ".fd", 0555, 1);
    append(programs, slot, ".var", 0555, 1);
    append(programs, slot, ".vfs", 0555, 1);
    programs->revision++;
    return 1;
}


/* EN: Check that a component is valid and not already published in this directory.
 * RU: Проверить допустимость компонента и отсутствие его в каталоге. */
static int free_name(const StyxRpcPrograms *programs, uint64_t parent, const char *name)
{
    size_t count = name ? strlen(name) : 0;
    if (!parent || !count || count > STYXRPC_TREE_NAME || strchr(name, '/') ||
        !strcmp(name, ".") || !strcmp(name, ".."))
    {
        return 0;
    }
    for (size_t i = 1; i < programs->used; ++i)
    {
        const StyxRpcRegistration *node = &programs->nodes[i];
        if (node->parent == parent && !strcmp(node->name, name))
        {
            return 0;
        }
    }
    return 1;
}


/* EN: Reserve a unique node with the exact announced byte access.
 * RU: Выделить уникальный узел с точным объявленным доступом. */
static StyxRpcRegistration *add_entry(StyxRpcPrograms *programs, uint64_t parent, const char *name,
    unsigned access, uint64_t length)
{
    if (!room(programs, 1) || !free_name(programs, parent, name) || !access ||
        (access & ~(STYXRPC_READ | STYXRPC_WRITE)))
    {
        return NULL;
    }
    unsigned mode = ((access & STYXRPC_READ) ? 0444 : 0) | ((access & STYXRPC_WRITE) ? 0222 : 0);
    StyxRpcRegistration *node = append(programs, parent, name, mode, 0);
    node->length = length;
    programs->revision++;
    return node;
}


/* EN: Validate resource callbacks before making a file visible.
 * RU: Проверить колбэки ресурса до публикации файла. */
static int add_resource(StyxRpcPrograms *programs, uint64_t parent, const char *name, unsigned access,
    uint64_t length, const StyxRpcResource *resource)
{
    if (!resource || ((access & STYXRPC_READ) && !resource->read) ||
        ((access & STYXRPC_WRITE) && !resource->write))
    {
        return 0;
    }
    StyxRpcRegistration *entry = add_entry(programs, parent, name, access, length);
    if (!entry)
    {
        return 0;
    }
    entry->resource = *resource;
    return 1;
}


/* EN/RU: See program.h. / См. program.h. */
int styxrpc_program_fd(StyxRpcPrograms *programs, uint32_t pid, unsigned fd, unsigned access,
    uint64_t length, const StyxRpcResource *resource)
{
    size_t node = find_process(programs, pid);
    if (!node || fd >= STYXRPC_FD_COUNT)
    {
        return 0;
    }
    char number[sizeof("31")];
    snprintf(number, sizeof(number), "%u", fd);
    return add_resource(programs, node + FD_CHILD, number, access, length, resource);
}


/* EN/RU: See program.h. / См. program.h. */
int styxrpc_program_variable(StyxRpcPrograms *programs, uint32_t pid, const char *name, unsigned access,
    uint64_t length, const StyxRpcResource *resource)
{
    size_t node = find_process(programs, pid);
    return node && add_resource(programs, node + VAR_CHILD, name, access, length, resource);
}


/* EN/RU: See program.h. / См. program.h. */
int styxrpc_program_file(StyxRpcPrograms *programs, uint32_t pid, const char *name, unsigned access,
    uint64_t length, const StyxRpcResource *resource)
{
    size_t node = find_process(programs, pid);
    return node && add_resource(programs, node + VFS_CHILD, name, access, length, resource);
}


/* EN/RU: See program.h. / См. program.h. */
int styxrpc_program_int(StyxRpcPrograms *programs, uint32_t pid, const char *name, const StyxIntVariable *variable)
{
    size_t node = find_process(programs, pid);
    if (!node || !variable)
    {
        return 0;
    }
    unsigned access = (variable->load ? STYXRPC_READ : 0) | (variable->store ? STYXRPC_WRITE : 0);
    StyxRpcRegistration *entry = add_entry(programs, node + VAR_CHILD, name, access, STYX_VARIABLE_INT_SIZE);
    if (!entry)
    {
        return 0;
    }
    entry->variable = *variable;
    return 1;
}


/* EN/RU: See program.h. / См. program.h. */
int styxrpc_program_unibus(StyxRpcPrograms *programs, uint32_t pid, const StyxRpcResource *resource)
{
    size_t node = find_process(programs, pid);
    return node && add_resource(programs, node, ".unibus", STYXRPC_READ | STYXRPC_WRITE, 0, resource);
}


/* EN/RU: See collection.h. / См. collection.h. */
int styxrpc_program_collection(StyxRpcPrograms *programs, uint32_t pid, const char *name, StyxIntArray *array)
{
    size_t process = find_process(programs, pid);
    if (!process || !array || !array->load || (array->contains && (array->insert || array->pop)) ||
        (array->add && !array->contains))
    {
        return 0;
    }
    uint64_t parent = process + VAR_CHILD;
    size_t required = 5 + (array->insert ? 2 : 0) + (array->pop != NULL) + (array->add != NULL);
    if (!room(programs, required) || !free_name(programs, parent, name))
    {
        return 0;
    }
    uint64_t directory = append(programs, parent, name, 0555, 1)->entry.id;
    const struct
    {
        const char *name; /* Exact Styx name. */
        unsigned mode;   /* Zero skips an unavailable mutation. */
    } members[] = {
        {".idx", 0222}, {".val", array->store ? 0666 : 0444}, {".len", 0444}, {".type", 0444},
        {".ins", array->insert ? 0222 : 0}, {".push", array->insert ? 0222 : 0},
        {".pop", array->pop ? 0444 : 0}, {".add", array->add ? 0222 : 0}
    };
    for (size_t i = 0; i < sizeof(members) / sizeof(*members); ++i)
    {
        if (members[i].mode)
        {
            StyxRpcRegistration *entry = append(programs, directory, members[i].name, members[i].mode, 0);
            entry->length = i == CollectionLength || i == CollectionType ? 0 : 4;
            if (i == CollectionInsert || i == CollectionAdd)
            {
                entry->length = 8;
            }
            styxrpc_collection_bind(entry, programs, array, (CollectionOperation)i);
        }
    }
    programs->revision++;
    return 1;
}


/* EN/RU: See bigtable.h. / См. bigtable.h. */
int styxrpc_program_bigtable(StyxRpcPrograms *programs, uint32_t pid, const char *name, const StyxRpcBigTable *table)
{
    size_t process = find_process(programs, pid);
    if (!process || !table || !table->apply)
    {
        return 0;
    }
    uint64_t parent = process + VAR_CHILD;
    size_t required = table->writable ? 10 : 6;
    if (!room(programs, required) || !free_name(programs, parent, name))
    {
        return 0;
    }
    StyxRpcRegistration *directory = append(programs, parent, name, 0555, 1);
    directory->table_spec = table;
    const struct
    {
        const char *name; /* Stock command basename. */
        unsigned mode;   /* Static access ceiling. */
    } files[] = {
        {".idx", 0222}, {".val", table->writable ? 0666 : 0444}, {".len", 0444},
        {".count", 0444}, {".type", 0444}, {".ins", 0222}, {".push", 0222},
        {".pop", 0444}, {".add", 0222}
    };
    for (size_t i = 0; i + 1 < required; ++i)
    {
        StyxRpcRegistration *entry = append(programs, directory->entry.id, files[i].name, files[i].mode, 0);
        entry->length = i == BigValue || i == BigPop ? 8 : 0;
        styxrpc_bigtable_bind(entry, directory, (BigNode)i);
    }
    programs->revision++;
    return 1;
}


/* EN: Return one registered child in stable insertion order.
 * RU: Вернуть одного потомка в устойчивом порядке регистрации. */
static const StyxRpcEntry *enumerate(void *context, uint64_t directory, size_t index)
{
    const StyxRpcPrograms *programs = context;
    for (size_t i = 1; i < programs->used; ++i)
    {
        if (programs->nodes[i].parent == directory && !index--)
        {
            return &programs->nodes[i].entry;
        }
    }
    return NULL;
}


/* EN: Borrow stable metadata by actual Qid, not by a client-supplied path.
 * RU: Заимствовать устойчивые метаданные по Qid, а не по пути клиента. */
static int lookup(void *context, uint64_t id, StyxRpcNode *out)
{
    const StyxRpcPrograms *programs = context;
    if (id >= programs->used)
    {
        return 0;
    }
    const StyxRpcRegistration *entry = &programs->nodes[id];
    *out = (StyxRpcNode){entry->entry, entry->parent, entry->length,
        entry->entry.directory ? programs->revision : 0};
    return 1;
}


typedef struct OpenResource
{
    const StyxRpcRegistration *entry; /* Borrowed until close. */
    void *cookie;                    /* Exact callback-owned handle. */
} OpenResource;


/* EN: Bind the current fid to registered data and its optional open callback.
 * RU: Закрепить данные регистрации и необязательный колбэк открытия за fid. */
static int open_resource(void *context, uint64_t id, unsigned access, void **output)
{
    const StyxRpcPrograms *programs = context;
    if (id >= programs->used || programs->nodes[id].entry.directory)
    {
        return 0;
    }
    const StyxRpcRegistration *entry = &programs->nodes[id];
    OpenResource *file = malloc(sizeof(*file));
    if (!file)
    {
        return 0;
    }
    *file = (OpenResource){entry, entry->resource.context};
    if (entry->resource.open && entry->resource.open(entry->resource.context, access, &file->cookie) != 1)
    {
        free(file);
        return 0;
    }
    *output = file;
    return 1;
}


/* EN: Deliver a callback, one network-order integer or a PID byte slice.
 * RU: Вызвать колбэк чтения либо выдать int или часть текста PID. */
static int read_resource(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    OpenResource *file = cookie;
    const StyxRpcRegistration *entry = file->entry;
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
    if (file->entry->resource.close)
    {
        file->entry->resource.close(file->cookie);
    }
    free(file);
}

const StyxRpcTree styxrpc_program_tree = {{enumerate, open_resource, read_resource, write_resource, close_resource}, lookup};
