/* Copyright (c) 2026 bhgv
 * EN: Native-owned namespace groups and versioned path plans; no VM/OS types.
 * RU: Native-группы namespace и планы путей с версиями; без типов VM/ОС.
 */
#ifndef DVM_VFS_NAMESPACE_H
#define DVM_VFS_NAMESPACE_H
#include <stdint.h>

#define DVM_NS_BEFORE 1U /* Matched Sys.MBEFORE: insert before existing branches. */
#define DVM_NS_AFTER 2U /* Matched Sys.MAFTER: append after existing branches. */
#define DVM_NS_CREATE 4U /* Matched Sys.MCREATE: branch may create missing entries. */
#define DVM_NS_ORDER 3U /* Ordering bits; combined BEFORE|AFTER is invalid. */
#define DVM_NS_PATH_LIMIT 4096 /* UTF-8 bytes including NUL; matches host metadata capacity. */
typedef struct DVM_Namespace DVM_Namespace;


/* EN: A copied plan owns no pointers; retain the source group until commit.
 * RU: Скопированный план не владеет указателями; держать группу до commit. */
typedef struct DVM_NamespacePlan
{
    char path[DVM_NS_PATH_LIMIT];   /* Absolute virtual spelling, lexically normalized. */
    char native[DVM_NS_PATH_LIMIT]; /* Frozen host path; no second namespace lookup. */
    uint64_t version;               /* Group revision observed while resolving both fields. */
} DVM_NamespacePlan;


/* EN: Create a group rooted at absolute native root with virtual cwd; return
 * owned reference or NULL on invalid paths/OOM. No host filesystem access.
 * load_directory is a virtual path relative to cwd (NULL uses cwd).
 * RU: Создать группу с абсолютным native root и virtual cwd; вернуть ссылку
 * либо NULL при неверных путях/OOM. load_directory — virtual path от cwd,
 * NULL означает cwd. Без доступа к host ФС. */
DVM_Namespace *dvm_namespace_create(const char *root, const char *cwd, const char *load_directory);


/* EN/RU: Retain borrowed live group; one on success, zero on count overflow.
 * Удержать живую группу; один при успехе, ноль при переполнении счётчика. */
int dvm_namespace_retain(DVM_Namespace *group);


/* EN/RU: Consume one group reference (NULL allowed); destroy at last owner.
 * Поглотить ссылку (NULL допустим); удалить при последнем владельце. */
void dvm_namespace_release(DVM_Namespace *group);


/* EN/RU: Copy a coherent borrowed group under its mutex; owned result or NULL.
 * Скопировать согласованную группу под её mutex; своя ссылка либо NULL. */
DVM_Namespace *dvm_namespace_copy(DVM_Namespace *group);


/* EN: Resolve borrowed path into caller-owned plan under group mutex. Return
 * one, or zero for empty/oversized paths. No I/O; caller retains group.
 * RU: Разрешить path в план caller под mutex группы. Один либо ноль для
 * пустого/слишком длинного пути. Без I/O; caller удерживает группу. */
int dvm_namespace_plan(DVM_Namespace *group, const char *path, DVM_NamespacePlan *plan);


/* EN: Resolve path from the group's import directory into a frozen plan.
 * Ownership/errors match ordinary plan; absolute paths use the same bindings.
 * RU: Разрешить path от import directory группы в frozen plan. Владение/ошибки
 * как у обычного plan; absolute paths используют те же bindings. */
int dvm_namespace_module_plan(DVM_Namespace *group, const char *path, DVM_NamespacePlan *plan);


/* EN: Commit validated directory plan as cwd. Return one on success, zero on
 * stale revision, -1 on revision exhaustion. No I/O; caller retains group.
 * RU: Применить проверенный directory plan как cwd. Один при успехе, ноль
 * при устаревшей версии, -1 при исчерпании версий. Без I/O; держать группу. */
int dvm_namespace_chdir(DVM_Namespace *group, const DVM_NamespacePlan *plan);


/* EN: Replace exact target with frozen source. Plans must come from this group
 * at one revision. Return 1/0/-1 for success/stale/OOM or revision exhaustion.
 * No I/O; previous open paths/handles remain independent of this mutation.
 * RU: Заменить точный target сохранённым source. Планы — из одной версии
 * этой группы. Вернуть 1/0/-1: успех/устарело/OOM либо исчерпание версий.
 * Без I/O; ранее открытые пути/handles независимы от изменения. */
int dvm_namespace_bind(DVM_Namespace *group, const DVM_NamespacePlan *source, const DVM_NamespacePlan *target);


/* EN: Remove exact target, optionally matching frozen native source. Return
 * 1/0/-1 for success/stale/absent or exhausted revision. Caller retains group.
 * RU: Удалить точный target, опционально сверив native source. 1/0/-1:
 * успех/устарело/нет записи либо исчерпаны версии. Caller держит группу. */
int dvm_namespace_unmount(DVM_Namespace *group, const DVM_NamespacePlan *target, const DVM_NamespacePlan *source);
#endif
