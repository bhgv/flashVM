/* Copyright (c) 2026 bhgv
 * EN: Shared virtual directory layout for hosted RPC modules.
 * RU: Общая структура виртуального каталога хостовых RPC-модулей.
 */
#ifndef STYXRPC_MODULE_NODES_H
#define STYXRPC_MODULE_NODES_H
#include "styxrpc/serve.h"

enum
{
    ROOT = 0,     /* Server logical root. */
    MOUNTED,      /* Directory of published modules. */
    MODULE,       /* Readable and traversable module directory. */
    INTERFACE,    /* Immutable compiled profile. */
    FIRST_EXPORT  /* First read/write/execute function file. */
};


/* EN: Validate the single module path component.
 * RU: Проверить один компонент пути модуля. */
int styxrpc_module_name(const char *name);


/* EN: Fill the directory entries preceding the function files.
 * RU: Заполнить структуру каталогов перед файлами функций. */
void styxrpc_module_nodes(StyxRpcEntry *entries, const char *name);


/* EN: Fill one function entry with the permissions expected by typed Dis load.
 * RU: Заполнить файл функции с правами, ожидаемыми типизированным load Dis. */
void styxrpc_module_export_node(StyxRpcEntry *entries, size_t index, const char *name);


/* EN: Borrow one immediate child by directory ID and index, or NULL at end.
 * RU: Заимствовать потомка по ID каталога и индексу или NULL в конце. */
const StyxRpcEntry *styxrpc_module_child(const StyxRpcEntry *entries, size_t count,
    uint64_t directory, size_t index);


/* EN: Copy checked metadata for an ID, returning zero for an unknown node.
 * RU: Скопировать метаданные по ID, при неизвестном узле вернуть ноль. */
int styxrpc_module_stat(const StyxRpcEntry *entries, size_t count, size_t profile_size,
    uint64_t id, StyxRpcNode *node);
#endif
