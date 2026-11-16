/* Copyright (c) 2026 bhgv
 * EN: Private per-group lock, references and native binding records.
 * RU: Приватный mutex группы, ссылки и native-записи привязок.
 */
#ifndef DVM_VFS_INTERNAL_H
#define DVM_VFS_INTERNAL_H
#include "namespace.h"
#include "channel_sync.h"
#include "resource.h"
#include <stddef.h>


/* EN/RU: One exact virtual prefix and its frozen host source, owned by group.
 * Один точный virtual prefix и сохранённый host source; владелец — группа. */
typedef struct DVM_NamespaceBinding
{
    uint64_t head_identity; /* Stable mount-head identity across stack changes/replacement. */
    uint64_t identity; /* Group-local layer identity; never reused. */
    unsigned flags; /* Inferno mount ordering/create flags. */
    char on[DVM_NS_PATH_LIMIT];        /* Virtual target; component boundaries matter. */
    char source[DVM_NS_PATH_LIMIT];    /* Native source captured when binding. */
    DVM_Provider *provider; /* Retained factory for a provider mount, otherwise NULL. */
    DVM_Anchor *anchor;                /* Owned parent/directory capability, NULL only in pure path tests. */
    char component[DVM_NS_PATH_LIMIT]; /* Empty for a bound directory, basename for a file. */
    struct DVM_NamespaceBinding *next; /* Next group-owned binding. */
} DVM_NamespaceBinding;


/* EN/RU: All mutable fields belong to lock; no managed pointers/GC roots.
 * Все изменяемые поля защищены lock; managed pointers/GC roots отсутствуют. */
struct DVM_Namespace
{
    DVM_NoticeHub *notices; /* Native namespace mutation push, never called under group lock. */
    FVM_PortSync *lock;                     /* Group-local mutex; never held across host I/O. */
    size_t references;                      /* Owning native references, including each resident. */
    uint64_t next_binding; /* Last assigned layer identity, protected by lock. */
    uint64_t version;                       /* Monotonic mutation revision; never wraps. */
    char root[DVM_NS_PATH_LIMIT];           /* Host path corresponding to virtual root. */
    char cwd[DVM_NS_PATH_LIMIT];            /* Current virtual directory shared by this group. */
    char load_directory[DVM_NS_PATH_LIMIT]; /* Initial package directory, follows cwd after chdir. */
    DVM_NamespaceBinding *bindings;         /* Owned list; longest matching component prefix wins. */
    DVM_Anchor *root_anchor;                /* Owned root directory identity. */
    DVM_Anchor *cwd_anchor;                 /* Owned current directory, independent of covering rebinds. */
    DVM_Anchor *load_anchor;                /* Owned import directory, initially entry package. */
    uint64_t cwd_identity; /* Reserved group-local identity for held cwd event subscription. */
    DVM_Provider *cwd_provider; /* Owned provider cwd, alternative to cwd_anchor. */
    DVM_Provider *load_provider; /* Owned import provider, follows successful chdir. */
    char cwd_component[DVM_NS_PATH_LIMIT]; /* Stable route prefix or dot in captured provider. */
    char load_component[DVM_NS_PATH_LIMIT]; /* Import route prefix paired with load_provider. */
    char cwd_source[DVM_NS_PATH_LIMIT];     /* Captured display spelling for cwd. */
    char load_source[DVM_NS_PATH_LIMIT];    /* Captured display spelling for import directory. */
};


/* EN: Normalize path relative to absolute cwd into distinct output. Return
 * one/zero; root clamps '..'. Empty and oversized paths fail, no allocation.
 * RU: Нормализовать path относительно absolute cwd в отдельный output.
 * Один/ноль; '..' ограничен корнем. Пустые/длинные пути отклоняются, без allocation. */
int dvm_namespace_normalize(const char *cwd, const char *path, char *output);


/* EN/RU: Link source-local cleanup after retained resolution; one/zero.
 * Подключить source-local cleanup после retained resolution; один/ноль. */
int dvm_resource_register(DVM_ResourcePlan *plan);

/* EN/RU: Remove exact revision/id/source or all target layers; no I/O under lock.
 * Удалить revision/id/source либо все target layers; без I/O под lock. */
int dvm_namespace_unmount_exact(DVM_Namespace *group, const DVM_NamespacePlan *target,
                                uint64_t id, const DVM_NamespacePlan *source);

/* EN/RU: Capture one current mount-head branch by index, retaining resource.
 * Return one/zero; output initially empty. No callbacks or host I/O under lock.
 * Захватить current ветвь mount-head по index; удержать resource. Один/ноль. */
int dvm_namespace_layer_at(DVM_Namespace *group, const char *on, uint64_t head, size_t index,
                            DVM_ResourcePlan *output);

/* EN/RU: Read finite current head length for one unionread call; no callbacks/I/O.
 * Получить finite current head length для одного unionread; без callbacks/I/O. */
size_t dvm_namespace_layer_count(DVM_Namespace *group, const char *on, uint64_t head);

/* EN/RU: Push committed namespace revision outside group lock; no target execution.
 * Передать committed revision вне group lock; без исполнения target. */
void dvm_namespace_changed(DVM_Namespace *group);
#endif
