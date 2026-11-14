/* Copyright (c) 2026 bhgv
 * EN: Private per-group lock, references and native binding records.
 * RU: Приватный mutex группы, ссылки и native-записи привязок.
 */
#ifndef DVM_VFS_INTERNAL_H
#define DVM_VFS_INTERNAL_H
#include "namespace.h"
#include "channel_sync.h"
#include <stddef.h>


/* EN/RU: One exact virtual prefix and its frozen host source, owned by group.
 * Один точный virtual prefix и сохранённый host source; владелец — группа. */
typedef struct DVM_NamespaceBinding
{
    char on[DVM_NS_PATH_LIMIT];        /* Virtual target; component boundaries matter. */
    char source[DVM_NS_PATH_LIMIT];    /* Native source captured when binding. */
    struct DVM_NamespaceBinding *next; /* Next group-owned binding. */
} DVM_NamespaceBinding;


/* EN/RU: All mutable fields belong to lock; no managed pointers/GC roots.
 * Все изменяемые поля защищены lock; managed pointers/GC roots отсутствуют. */
struct DVM_Namespace
{
    FVM_PortSync *lock;                     /* Group-local mutex; never held across host I/O. */
    size_t references;                      /* Owning native references, including each resident. */
    uint64_t version;                       /* Monotonic mutation revision; never wraps. */
    char root[DVM_NS_PATH_LIMIT];           /* Host path corresponding to virtual root. */
    char cwd[DVM_NS_PATH_LIMIT];            /* Current virtual directory shared by this group. */
    char load_directory[DVM_NS_PATH_LIMIT]; /* Initial package directory, follows cwd after chdir. */
    DVM_NamespaceBinding *bindings;         /* Owned list; longest matching component prefix wins. */
};


/* EN: Normalize path relative to absolute cwd into distinct output. Return
 * one/zero; root clamps '..'. Empty and oversized paths fail, no allocation.
 * RU: Нормализовать path относительно absolute cwd в отдельный output.
 * Один/ноль; '..' ограничен корнем. Пустые/длинные пути отклоняются, без allocation. */
int dvm_namespace_normalize(const char *cwd, const char *path, char *output);
#endif
