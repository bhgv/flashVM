/* Copyright (c) 2026 bhgv
 * EN: Route stable control nodes and generation-scoped module fids on one server.
 * RU: Маршрутизация постоянных узлов управления и fid образа по поколениям на одном сервере.
 */
#include "styxrpc/module_slot.h"
#include "control_internal.h"
#include <stdlib.h>

#define IMAGE_SHIFT 32U /* High Qid bits identify the image generation. */

typedef struct SlotFile SlotFile; /* One open on the current image or a control node. */
struct SlotFile
{
    StyxRpcFile file;          /* Owned nested fid state; cleared on image retirement. */
    SlotFile *next;            /* Next live image open; NULL for a control or retired file. */
    SlotFile **previous;      /* Owning link, or NULL for a control or retired file. */
};

struct StyxRpcModuleSlot
{
    StyxRpcControl *control; /* Borrowed stable control tree context. */
    StyxRpcModule *module;    /* Borrowed image through retirement. */
    uint32_t generation;      /* Monotonic image identity and root revision. */
    SlotFile *opened;         /* Fids for the current image. */
    StyxRpcEntry entry;       /* Borrowed enumeration result storage. */
};


/* EN/RU: See module_slot.h. / См. module_slot.h. */
StyxRpcModuleSlot *styxrpc_module_slot_create(StyxRpcControl *control)
{
    StyxRpcModuleSlot *slot = control ? calloc(1, sizeof(*slot)) : NULL;
    if (slot)
    {
        slot->control = control;
    }
    return slot;
}


/* EN/RU: See module_slot.h. / См. module_slot.h. */
StyxRpcStatus styxrpc_module_slot_set(StyxRpcModuleSlot *slot, StyxRpcModule *module)
{
    if (!slot || (module && slot->generation >= UINT32_MAX - 1))
    {
        return STYXRPC_INVALID;
    }
    if (module == slot->module)
    {
        return STYXRPC_OK;
    }
    while (slot->opened)
    {
        SlotFile *file = slot->opened;
        slot->opened = file->next;
        file->previous = NULL;
        file->next = NULL;
        styxrpc_file_close(&file->file);
    }
    ++slot->generation;
    slot->module = module;
    return STYXRPC_OK;
}


/* EN/RU: See module_slot.h. / См. module_slot.h. */
void styxrpc_module_slot_destroy(StyxRpcModuleSlot *slot)
{
    free(slot);
}


/* EN: Build the full identity for a node in the live image.
 * RU: Собрать полный идентификатор узла действующего образа. */
static uint64_t image_id(const StyxRpcModuleSlot *slot, uint64_t local)
{
    return ((uint64_t)slot->generation << IMAGE_SHIFT) | local;
}


/* EN: Enumerate stable controls followed by the image's mounted directory, if any.
 * RU: Перечислить управление, затем mounted действующего образа при его наличии. */
static const StyxRpcEntry *enumerate(void *context, uint64_t directory, size_t index)
{
    StyxRpcModuleSlot *slot = context;
    uint64_t generation = directory >> IMAGE_SHIFT;
    if (!generation)
    {
        const StyxRpcEntry *entry = styxrpc_control_tree.files.enumerate(slot->control, directory, index);
        if (entry || directory)
        {
            return entry;
        }
        if (index != CONTROL_STATE)
        {
            return NULL;
        }
        index = 0;
    }
    if (!slot->module || (generation && generation != slot->generation))
    {
        return NULL;
    }
    const StyxRpcEntry *entry = styxrpc_module_tree.files.enumerate(slot->module, (uint32_t)directory, index);
    if (!entry)
    {
        return NULL;
    }
    slot->entry = *entry;
    slot->entry.id = image_id(slot, entry->id);
    return &slot->entry;
}


/* EN: Resolve an exact Qid and remap its parent, preserving the stable root.
 * RU: Разрешить точный Qid и родителя с сохранением постоянного корня. */
static int describe(void *context, uint64_t id, StyxRpcNode *node)
{
    StyxRpcModuleSlot *slot = context;
    uint64_t generation = id >> IMAGE_SHIFT;
    if (!generation)
    {
        if (!styxrpc_control_tree.stat(slot->control, id, node))
        {
            return 0;
        }
        if (!id)
        {
            node->revision = slot->generation;
        }
        return 1;
    }
    if (!slot->module || generation != slot->generation || !(uint32_t)id ||
        !styxrpc_module_tree.stat(slot->module, (uint32_t)id, node))
    {
        return 0;
    }
    node->entry.id = id;
    if (node->parent)
    {
        node->parent = image_id(slot, node->parent);
    }
    return 1;
}


/* EN: Open exactly the requested generation and link image fids for immediate retirement.
 * RU: Открыть запрошенное поколение и учесть fid образа для немедленного отзыва. */
static int open_file(void *context, uint64_t id, unsigned access, void **cookie)
{
    StyxRpcModuleSlot *slot = context;
    uint64_t generation = id >> IMAGE_SHIFT;
    if (generation && (!slot->module || generation != slot->generation))
    {
        return 0;
    }
    SlotFile *opened = calloc(1, sizeof(*opened));
    if (!opened)
    {
        return 0;
    }
    const StyxRpcTree *tree = generation ? &styxrpc_module_tree : &styxrpc_control_tree;
    void *data = generation ? (void *)slot->module : slot->control;
    if (styxrpc_file_open(&opened->file, &tree->files, data, (uint32_t)id, access) != STYXRPC_OK)
    {
        free(opened);
        return 0;
    }
    if (generation)
    {
        opened->next = slot->opened;
        opened->previous = &slot->opened;
        if (opened->next)
        {
            opened->next->previous = &opened->next;
        }
        slot->opened = opened;
    }
    *cookie = opened;
    return 1;
}


/* EN: Read the original open or refuse a retired image fid.
 * RU: Читать исходный файл либо отклонить отозванный fid образа. */
static int read_file(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *used)
{
    SlotFile *opened = cookie;
    return styxrpc_file_read(&opened->file, offset, bytes, capacity, used) == STYXRPC_OK;
}


/* EN: Write the original open or refuse a retired image fid.
 * RU: Писать в исходный файл либо отклонить отозванный fid образа. */
static int write_file(void *cookie, uint64_t offset, const void *bytes, size_t count, size_t *used)
{
    SlotFile *opened = cookie;
    return styxrpc_file_write(&opened->file, offset, bytes, count, used) == STYXRPC_OK;
}


/* EN: Unlink and consume an outer fid, including already retired image fids.
 * RU: Исключить и освободить внешний fid, включая отозванный fid образа. */
static void close_file(void *cookie)
{
    SlotFile *opened = cookie;
    if (opened->previous)
    {
        *opened->previous = opened->next;
        if (opened->next)
        {
            opened->next->previous = opened->previous;
        }
    }
    styxrpc_file_close(&opened->file);
    free(opened);
}

const StyxRpcTree styxrpc_module_slot_tree = {{enumerate, open_file, read_file, write_file, close_file}, describe};
