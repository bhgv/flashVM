/* Copyright (c) 2026 bhgv
 * EN: Resolve module references against the owner's publication and keep exact fids.
 * RU: Разрешение ссылок в публикации владельца с удержанием точных fid.
 */
#include "styxrpc/module_ref.h"
#include "styxrpc/module_slot.h"
#include <string.h>


/* EN: Match a counted name below a live directory. Copy the entry on success.
 * RU: Найти имя с заданной длиной в живом каталоге. При успехе копировать запись. */
static int child(StyxRpcModuleSlot *slot, uint64_t directory, const char *name, size_t length,
    StyxRpcEntry *entry)
{
    const StyxRpcVfs *vfs = &styxrpc_module_slot_tree.files;
    for (size_t i = 0; i < STYXRPC_TREE_CHILDREN; ++i)
    {
        if (styxrpc_dir_entry(vfs, slot, directory, i, entry) != STYXRPC_OK)
        {
            return 0;
        }
        if (strlen(entry->name) == length && !memcmp(entry->name, name, length))
        {
            return 1;
        }
    }
    return 0;
}


/* EN: Require that every path component leads to the original typed export.
 * RU: Проверить, что все компоненты пути ведут к исходному типизированному экспорту. */
static int matches(StyxRpcModuleSlot *slot, const StyxRpcModuleRef *ref, unsigned access)
{
    uint64_t directory = 0;
    const char *name = ref->path;
    for (;;)
    {
        size_t length = strcspn(name, "/");
        StyxRpcEntry entry;
        if (!child(slot, directory, name, length, &entry))
        {
            return 0;
        }
        if (!name[length])
        {
            unsigned required = (access & STYXRPC_WRITE) ? 6U : 4U;
            StyxRpcNode node;
            return !entry.directory && entry.id == ref->node && (entry.mode & required) == required &&
                styxrpc_module_slot_tree.stat(slot, entry.id, &node) && node.length == STYX_VARIABLE_INT_SIZE;
        }
        if (!entry.directory || !(entry.mode & 1U))
        {
            return 0;
        }
        directory = entry.id;
        name += length + 1;
    }
}


/* EN/RU: See module_ref.h. / См. module_ref.h. */
StyxRpcStatus styxrpc_module_ref_open_local(StyxRpcModuleSlot *slot, const StyxRpcModuleRef *ref, StyxRpcFile *out)
{
    if (!slot || !ref || !out || !ref->node || (ref->writable != 0 && ref->writable != 1) ||
        !memchr(ref->path, 0, sizeof(ref->path)) || !styxrpc_module_ref_path(ref->path))
    {
        return STYXRPC_INVALID;
    }
    unsigned access = STYXRPC_READ | (ref->writable ? STYXRPC_WRITE : 0);
    return matches(slot, ref, access) ?
        styxrpc_file_open(out, &styxrpc_module_slot_tree.files, slot, ref->node, access) : STYXRPC_INVALID;
}


/* EN: Provide the value of an owner's borrowed signed scalar to the byte codec.
 * RU: Передать кодеку значение заимствованного знакового скаляра. */
static int copy_from(void *source, int32_t *out)
{
    *out = *(const int32_t *)source;
    return 1;
}


/* EN: Capture one signed scalar decoded from the held export file.
 * RU: Сохранить знаковый скаляр, декодированный из открытого файла экспорта. */
static int copy_to(void *target, int32_t value)
{
    *(int32_t *)target = value;
    return 1;
}


/* EN/RU: See module_ref.h. / См. module_ref.h. */
StyxRpcStatus styxrpc_module_ref_read_local(StyxRpcFile *file, int32_t *value)
{
    unsigned char bytes[STYX_VARIABLE_INT_SIZE];
    size_t size;
    if (!value || styxrpc_file_read(file, 0, bytes, sizeof(bytes), &size) != STYXRPC_OK)
    {
        return STYXRPC_INVALID;
    }
    const StyxIntVariable copy = {value, NULL, copy_to};
    return styxintvariablewrite(&copy, 0, bytes, size) ? STYXRPC_OK : STYXRPC_INVALID;
}


/* EN/RU: See module_ref.h. / См. module_ref.h. */
StyxRpcStatus styxrpc_module_ref_write_local(StyxRpcFile *file, int32_t value)
{
    unsigned char bytes[STYX_VARIABLE_INT_SIZE];
    size_t size = sizeof(bytes), used;
    const StyxIntVariable copy = {&value, copy_from, NULL};
    if (!styxintvariableread(&copy, 0, bytes, &size) ||
        styxrpc_file_write(file, 0, bytes, size, &used) != STYXRPC_OK || used != size)
    {
        return STYXRPC_INVALID;
    }
    return STYXRPC_OK;
}
