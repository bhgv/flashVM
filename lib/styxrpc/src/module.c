/* Copyright (c) 2026 bhgv
 * EN: Publish a single verified namespace over existing callback module profiles.
 * RU: Публикация одного проверенного пространства имён поверх профилей колбэков.
 */
#include "styxrpc/module.h"
#include "module_nodes.h"
#include "control_internal.h"
#include <stdlib.h>
#include <string.h>

enum
{
    CONTROL_OFFSET = FIRST_EXPORT + 2 * STYXRPC_MAX_EXPORTS + 4 /* IDs above all module functions and cells. */
};

typedef struct ModuleRoute
{
    const StyxRpcTree *tree; /* Exact callback profile for this file. */
    void *context;          /* Embedded owner module for the selected tree. */
    uint64_t id;            /* Private node identity in that tree. */
} ModuleRoute;

typedef struct ModuleFile
{
    const StyxRpcVfs *operations; /* Borrowed delegate, NULL for compiled interface reads. */
    void *cookie;                /* Owned delegate cursor. */
    const StyxRpcModule *module; /* Borrowed interface backing. */
    const StyxIntVariable *variable; /* Borrowed .var binding when a cell is open. */
    unsigned access;                /* Requested read and write rights for the cell. */
} ModuleFile;

struct StyxRpcModule
{
    char name[STYXRPC_TREE_NAME + 1]; /* Owned basename of this image. */
    char names[STYXRPC_MAX_EXPORTS][STYXRPC_TREE_NAME + 1]; /* Owned distinct export names. */
    StyxRpcEntry entries[FIRST_EXPORT + STYXRPC_MAX_EXPORTS + 4]; /* Merged public module nodes. */
    ModuleRoute routes[STYXRPC_MAX_EXPORTS + 3]; /* Private delegates from FIRST_EXPORT. */
    size_t count; /* Number of functions and markers. */
    size_t profile_size; /* Number of bytes in compiled interface. */
    unsigned char profile[STYXRPC_MANAGED_PROFILE_BYTES]; /* Merged compiled interface. */
    StyxRpcScalarModule scalar; /* Scalar codec and tree. */
    StyxRpcManagedModule managed; /* Managed codec and tree. */
    StyxRpcGraphModule graph; /* Positional graph codec and tree. */
    StyxRpcChannelModule channel; /* Live result codec and authenticated bus. */
    StyxRpcExport integers[STYXRPC_MAX_EXPORTS]; /* Copied scalar callbacks. */
    StyxRpcManagedExport values[STYXRPC_MAX_EXPORTS]; /* Copied managed callbacks. */
    StyxRpcGraphExport graphs[STYXRPC_MAX_EXPORTS]; /* Copied graph callbacks. */
    StyxRpcChannelExport channels[STYXRPC_MAX_EXPORTS]; /* Copied channel factories. */
    uint64_t environment; /* .var directory id, zero when absent. */
    char variable_names[STYXRPC_MAX_EXPORTS][STYXRPC_TREE_NAME + 1]; /* Owned cell names. */
    StyxRpcEntry variable_entries[STYXRPC_MAX_EXPORTS]; /* Immediate children of .var. */
    StyxIntVariable variables[STYXRPC_MAX_EXPORTS]; /* Copied variable callbacks. */
    size_t variable_count; /* Number of live cell descriptors. */
    StyxRpcControl *control; /* Borrowed lifecycle callbacks at the shared server root. */
    StyxRpcEntry control_entries[CONTROL_STATE]; /* Control node IDs remapped to avoid module collisions. */
};


/* EN: Select the exported name from the active descriptor.
 * RU: Получить имя экспорта из активного описателя. */
static const char *entry_name(const StyxRpcModuleEntry *entry)
{
    switch (entry->kind)
    {
    case STYXRPC_MODULE_INT: return entry->callback.integer.name;
    case STYXRPC_MODULE_MANAGED: return entry->callback.managed.name;
    case STYXRPC_MODULE_GRAPH: return entry->callback.graph.name;
    case STYXRPC_MODULE_CHANNEL: return entry->callback.channel.name;
    }
    return NULL;
}


/* EN: Copy one verified function into its delegate table and preserve its public order.
 * RU: Скопировать проверенную функцию в таблицу профиля, сохранив публичный порядок. */
static void add_function(StyxRpcModule *module, size_t public, const StyxRpcModuleEntry *entry, size_t slot)
{
    const char *name = module->names[public];
    ModuleRoute *route = module->routes + public;
    route->id = FIRST_EXPORT + slot;
    switch (entry->kind)
    {
    case STYXRPC_MODULE_INT:
        module->integers[slot] = entry->callback.integer;
        module->integers[slot].name = name;
        route->tree = &styxrpc_scalar_module_tree;
        route->context = &module->scalar;
        break;
    case STYXRPC_MODULE_MANAGED:
        module->values[slot] = entry->callback.managed;
        module->values[slot].name = name;
        route->tree = &styxrpc_managed_module_tree;
        route->context = &module->managed;
        break;
    case STYXRPC_MODULE_GRAPH:
        module->graphs[slot] = entry->callback.graph;
        module->graphs[slot].name = name;
        route->tree = &styxrpc_graph_module_tree;
        route->context = &module->graph;
        break;
    case STYXRPC_MODULE_CHANNEL:
        module->channels[slot] = entry->callback.channel;
        module->channels[slot].name = name;
        route->tree = &styxrpc_channel_module_tree;
        route->context = &module->channel;
        break;
    }
    styxrpc_module_export_node(module->entries, public, name);
}


/* EN: Append a full checked profile, preserving the total wire extent.
 * RU: Добавить проверенный профиль с учётом полной длины кадра. */
static int append_profile(StyxRpcModule *module, const unsigned char *bytes, size_t count)
{
    if (count > sizeof(module->profile) - module->profile_size)
    {
        return 0;
    }
    memcpy(module->profile + module->profile_size, bytes, count);
    module->profile_size += count;
    return 1;
}


/* EN: Reflect a standard metadata or bus file from its exact delegate.
 * RU: Отобразить стандартный файл метаданных либо шины из его точного профиля. */
static int add_marker(StyxRpcModule *module, const StyxRpcTree *tree, void *context, size_t source)
{
    StyxRpcNode node;
    if (!tree->stat(context, source, &node))
    {
        return 0;
    }
    size_t index = module->count++;
    module->routes[index] = (ModuleRoute){tree, context, source};
    node.entry.id = FIRST_EXPORT + index;
    module->entries[node.entry.id] = node.entry;
    return 1;
}


/* EN: Validate all profiles and emit only one copy of each shared protocol marker.
 * RU: Проверить все профили и выдать по одной копии общих маркеров протокола. */
static int install_profiles(StyxRpcModule *module, const size_t *counts, StyxRpcModuleChannels *registry)
{
    size_t ints = counts[STYXRPC_MODULE_INT], values = counts[STYXRPC_MODULE_MANAGED];
    size_t graphs = counts[STYXRPC_MODULE_GRAPH], channels = counts[STYXRPC_MODULE_CHANNEL];
    const char *name = module->name;
    if ((ints && styxrpc_scalar_module_init(&module->scalar, name, module->integers, ints) != STYXRPC_OK) ||
        (values && styxrpc_managed_module_init(&module->managed, name, module->values, values) != STYXRPC_OK) ||
        (graphs && styxrpc_graph_module_init(&module->graph, name, module->graphs, graphs) != STYXRPC_OK) ||
        (channels && styxrpc_channel_module_init(&module->channel, name, module->channels, channels, registry) != STYXRPC_OK) ||
        !append_profile(module, module->scalar.profile, module->scalar.profile_size) ||
        !append_profile(module, module->managed.profile, module->managed.profile_size) ||
        !append_profile(module, module->graph.profile, module->graph.profile_size) ||
        !append_profile(module, module->channel.graph.profile, module->channel.graph.profile_size))
    {
        return 0;
    }
    if (channels)
    {
        const StyxRpcTree *tree = &styxrpc_channel_module_tree;
        size_t marker = FIRST_EXPORT + channels;
        return add_marker(module, tree, &module->channel, marker) &&
            add_marker(module, tree, &module->channel, marker + 1) &&
            add_marker(module, tree, &module->channel, marker + 2);
    }
    return !graphs || add_marker(module, &styxrpc_graph_module_tree, &module->graph, FIRST_EXPORT + graphs);
}


/* EN: Publish one optional .var directory with copied names and exact access.
 * RU: Опубликовать необязательный каталог .var с копиями имён и точными правами. */
static int install_variables(StyxRpcModule *module, const StyxRpcModuleVariable *variables, size_t count)
{
    if (!count)
    {
        return 1;
    }
    module->environment = FIRST_EXPORT + module->count++;
    module->entries[module->environment] = (StyxRpcEntry){module->environment, ".var", 0555, 1};
    for (size_t i = 0; i < count; ++i)
    {
        const StyxRpcModuleVariable *variable = variables + i;
        unsigned mode = (variable->value.load ? 0444 : 0) | (variable->value.store ? 0222 : 0);
        if (!mode || !styxrpc_module_name(variable->name))
        {
            return 0;
        }
        for (size_t j = 0; j < i; ++j)
        {
            if (!strcmp(variable->name, module->variable_names[j]))
            {
                return 0;
            }
        }
        strcpy(module->variable_names[i], variable->name);
        module->variables[i] = variable->value;
        module->variable_entries[i] = (StyxRpcEntry){module->environment + 1 + i,
            module->variable_names[i], mode, 0};
    }
    module->variable_count = count;
    return 1;
}


/* EN/RU: See styxrpc/module.h. / См. styxrpc/module.h. */
StyxRpcStatus styxrpc_module_create(const StyxRpcModuleSpec *spec, StyxRpcModule **out)
{
    if (!out || !spec || !styxrpc_module_name(spec->name) || !spec->entries || !spec->count ||
        spec->count > STYXRPC_MAX_EXPORTS || spec->variable_count > STYXRPC_MAX_EXPORTS ||
        (spec->variable_count && !spec->variables))
    {
        return STYXRPC_INVALID;
    }
    const StyxRpcModuleEntry *entries = spec->entries;
    size_t count = spec->count;
    for (size_t i = 0; i < count; ++i)
    {
        const char *exported = entry_name(entries + i);
        if (!styxrpc_module_name(exported) || exported[0] == '.')
        {
            return STYXRPC_INVALID;
        }
        for (size_t j = 0; j < i; ++j)
        {
            if (!strcmp(exported, entry_name(entries + j)))
            {
                return STYXRPC_INVALID;
            }
        }
    }
    StyxRpcModule *module = calloc(1, sizeof(*module));
    if (!module)
    {
        return STYXRPC_INVALID;
    }
    strcpy(module->name, spec->name);
    styxrpc_module_nodes(module->entries, module->name);
    size_t counts[STYXRPC_MODULE_CHANNEL + 1] = {0};
    for (size_t i = 0; i < count; ++i)
    {
        strcpy(module->names[i], entry_name(entries + i));
        add_function(module, i, entries + i, counts[entries[i].kind]++);
    }
    module->count = count;
    if (!install_profiles(module, counts, spec->registry) ||
        !install_variables(module, spec->variables, spec->variable_count))
    {
        free(module);
        return STYXRPC_INVALID;
    }
    module->control = spec->control;
    if (module->control)
    {
        for (size_t i = 0; i < CONTROL_STATE; ++i)
        {
            module->control_entries[i] = *styxrpc_control_tree.files.enumerate(module->control, CONTROL_ROOT, i);
            module->control_entries[i].id += CONTROL_OFFSET;
        }
    }
    *out = module;
    return STYXRPC_OK;
}


/* EN/RU: See styxrpc/module.h. / См. styxrpc/module.h. */
void styxrpc_module_destroy(StyxRpcModule *module)
{
    free(module);
}


/* EN: Borrow a child of the merged tree in public order.
 * RU: Заимствовать дочерний узел объединённого дерева в публичном порядке. */
static const StyxRpcEntry *enumerate(void *context, uint64_t directory, size_t index)
{
    const StyxRpcModule *module = context;
    if (module->environment && directory == module->environment)
    {
        return index < module->variable_count ? module->variable_entries + index : NULL;
    }
    if (module->control && directory == ROOT && index > 0 && index <= CONTROL_STATE)
    {
        return module->control_entries + index - 1;
    }
    return styxrpc_module_child(module->entries, module->count, directory, index);
}


/* EN: Copy public metadata and retain the delegate's reported file length.
 * RU: Скопировать публичные метаданные и длину файла делегата. */
static int describe(void *context, uint64_t id, StyxRpcNode *node)
{
    const StyxRpcModule *module = context;
    if (module->environment && id > module->environment && id - module->environment <= module->variable_count)
    {
        *node = (StyxRpcNode){.entry = module->variable_entries[id - module->environment - 1],
            .parent = module->environment, .length = STYX_VARIABLE_INT_SIZE};
        return 1;
    }
    if (module->control && id > CONTROL_OFFSET && id - CONTROL_OFFSET <= CONTROL_STATE)
    {
        *node = (StyxRpcNode){.entry = module->control_entries[id - CONTROL_OFFSET - 1], .parent = ROOT};
        return 1;
    }
    if (!styxrpc_module_stat(module->entries, module->count, module->profile_size, id, node))
    {
        return 0;
    }
    if (id >= FIRST_EXPORT && id != module->environment)
    {
        const ModuleRoute *route = module->routes + id - FIRST_EXPORT;
        StyxRpcNode original;
        if (!route->tree->stat(route->context, route->id, &original))
        {
            return 0;
        }
        node->length = original.length;
    }
    return 1;
}


/* EN: Open one private fid on the selected profile, or expose the immutable interface.
 * RU: Открыть приватный fid выбранного профиля либо выдать неизменяемый интерфейс. */
static int open_file(void *context, uint64_t id, unsigned access, void **cookie)
{
    const StyxRpcModule *module = context;
    int variable = module->environment && id > module->environment && id - module->environment <= module->variable_count;
    int control = module->control && id > CONTROL_OFFSET && id - CONTROL_OFFSET <= CONTROL_STATE;
    if (id < INTERFACE || id == module->environment || (!control && !variable && id >= FIRST_EXPORT + module->count) ||
        (id == INTERFACE && access != STYXRPC_READ))
    {
        return 0;
    }
    ModuleFile *file = calloc(1, sizeof(*file));
    if (!file)
    {
        return 0;
    }
    file->module = module;
    if (variable)
    {
        const StyxIntVariable *cell = module->variables + id - module->environment - 1;
        unsigned allowed = (cell->load ? STYXRPC_READ : 0) | (cell->store ? STYXRPC_WRITE : 0);
        if (!access || (access & ~allowed))
        {
            free(file);
            return 0;
        }
        file->variable = cell;
        file->access = access;
    }
    else if (id >= FIRST_EXPORT)
    {
        ModuleRoute selected = control ? (ModuleRoute){&styxrpc_control_tree, module->control, id - CONTROL_OFFSET} :
            module->routes[id - FIRST_EXPORT];
        file->operations = &selected.tree->files;
        if (!file->operations->open(selected.context, selected.id, access, &file->cookie))
        {
            free(file);
            return 0;
        }
    }
    *cookie = file;
    return 1;
}


/* EN: Read a confirmed reply or a slice of the compiled interface from the held open.
 * RU: Прочитать подтверждённый ответ либо фрагмент интерфейса из открытого fid. */
static int read_file(void *state, uint64_t offset, void *bytes, size_t capacity, size_t *used)
{
    const ModuleFile *file = state;
    if (file->variable)
    {
        *used = capacity;
        return (file->access & STYXRPC_READ) && styxintvariableread(file->variable, offset, bytes, used);
    }
    if (file->operations)
    {
        return file->operations->read(file->cookie, offset, bytes, capacity, used);
    }
    size_t remaining = offset < file->module->profile_size ? file->module->profile_size - (size_t)offset : 0;
    *used = remaining < capacity ? remaining : capacity;
    if (*used)
    {
        memcpy(bytes, file->module->profile + (size_t)offset, *used);
    }
    return 1;
}


/* EN: Submit bytes only to the held function or bus delegate.
 * RU: Передать байты только удерживаемой функции либо её шине. */
static int write_file(void *state, uint64_t offset, const void *bytes, size_t size, size_t *used)
{
    ModuleFile *file = state;
    if (file->variable)
    {
        if (!(file->access & STYXRPC_WRITE) || !styxintvariablewrite(file->variable, offset, bytes, size))
        {
            return 0;
        }
        *used = size;
        return 1;
    }
    return file->operations && file->operations->write(file->cookie, offset, bytes, size, used);
}


/* EN: Consume the exact delegate fid before releasing the outer cookie.
 * RU: Освободить точный fid профиля перед внешним состоянием открытия. */
static void close_file(void *state)
{
    ModuleFile *file = state;
    if (file->operations)
    {
        file->operations->close(file->cookie);
    }
    free(file);
}

const StyxRpcTree styxrpc_module_tree = {{enumerate, open_file, read_file, write_file, close_file}, describe}; /* Shared image dispatcher. */
