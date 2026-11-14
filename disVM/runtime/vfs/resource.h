/* Copyright (c) 2026 bhgv
 * EN: Refcounted directory resources and owner-local I/O plans, never managed roots.
 * RU: Refcounted directory resources и owner-local I/O plans; не managed roots.
 */
#ifndef DVM_VFS_RESOURCE_H
#define DVM_VFS_RESOURCE_H
#include "namespace.h"
#include "anchor_port.h"
#include "disvm/native_cleanup.h"
typedef struct DVM_Anchor DVM_Anchor; /* Native capability with independently retained users. */


/* EN: Stable caller storage until close; never copy a live cleanup record.
 * RU: Стабильная caller storage до close; не копировать живой cleanup record. */
typedef struct DVM_ResourcePlan
{
    DVM_NamespacePlan names;          /* Frozen virtual/display names and group revision. */
    char relative[DVM_NS_PATH_LIMIT]; /* Selected path relative to anchor, or empty for native fallback. */
    DVM_Anchor *anchor;               /* Owned selection retained under the group lock. */
    DVM_Anchor *created;              /* Owned new directory until commit retains it. */
    DVM_NativeCleanup cleanup;        /* Current native owner's unwind registration. */
} DVM_ResourcePlan;


/* EN/RU: Open owned native directory, possibly relative to borrowed parent;
 * return NULL on error. Открыть свой каталог от parent либо native path;
 * NULL при ошибке. Native-only blocking; no VM access. */
DVM_Anchor *dvm_anchor_open(DVM_Anchor *parent, const char *path);


/* EN/RU: Retain live anchor; one/zero for success/overflow. No I/O.
 * Удержать anchor; один/ноль для успеха/overflow. Без I/O. */
int dvm_anchor_retain(DVM_Anchor *anchor);


/* EN/RU: Consume reference, closing only last; NULL allowed, no managed access.
 * Поглотить ссылку, закрыть только последнюю; NULL допустим, без managed access. */
void dvm_anchor_release(DVM_Anchor *anchor);


/* EN/RU: Borrow immutable port handle while owning anchor reference.
 * Заимствовать port handle, удерживая anchor reference. */
FVM_PortAnchor *dvm_anchor_port(DVM_Anchor *anchor);


/* EN: Capture group/path (module selects import base) and retain resource.
 * Return one/zero, registering source-local cleanup when a context exists.
 * Caller supplies stable uninitialized plan, then closes on every return.
 * RU: Захватить group/path (module выбирает import base), удержать resource.
 * Один/ноль; при наличии context регистрируется source-local cleanup.
 * Caller даёт стабильный неинициализированный plan и закрывает на всех выходах. */
int dvm_resource_plan(DVM_Namespace *group, const char *path, int module, DVM_ResourcePlan *plan);


/* EN/RU: Close plan on its source owner, also used by native unwind; NULL harmless.
 * Закрыть plan своим source owner; используется native unwind; NULL безопасен. */
void dvm_resource_close(DVM_ResourcePlan *plan);


/* EN/RU: Stat frozen selection; output native, plan borrowed through blocking I/O.
 * Stat frozen selection; native output, plan заимствован до конца blocking I/O. */
FVM_Result dvm_resource_info(DVM_ResourcePlan *plan, FVM_FileInfo *output);


/* EN/RU: Open frozen selection in mode, transfer file/cursor on success.
 * Открыть frozen selection в mode; передать file/cursor при успехе. Native-only. */
FVM_Result
dvm_resource_open(DVM_ResourcePlan *plan, const char *mode, FVM_PortFile **file, FVM_PortDirectory **directory);


/* EN/RU: Remove selected native entry; plan/anchor borrowed through native I/O.
 * No fallback reopening stale display path; no recursive directory deletion.
 * Удалить selected native entry; plan/anchor заимствованы до конца I/O.
 * Без fallback через stale display path и recursive удаления. Return port status. */
FVM_Result dvm_resource_remove(DVM_ResourcePlan *plan);


/* EN/RU: Pin startup root/cwd/import resources before group publication; one/zero.
 * Закрепить startup root/cwd/import до публикации группы; один/ноль. */
int dvm_namespace_pin(DVM_Namespace *group);


/* EN/RU: Commit borrowed plan.created as cwd; 1 success, 0 stale, -1 error.
 * Применить plan.created как cwd; 1 успех, 0 stale, -1 ошибка. Retains refs; no I/O. */
int dvm_namespace_chdir_resource(DVM_Namespace *group, DVM_ResourcePlan *plan);


/* EN/RU: Bind source/target revision and borrowed anchor/component; retain on success.
 * 1 success, 0 stale, -1 allocation/revision error. No I/O under group lock.
 * Связать source/target и anchor/component; удержать при успехе.
 * 1 успех, 0 stale, -1 allocation/revision error. Без I/O под group mutex. */
int dvm_namespace_bind_resource(DVM_Namespace *group,
                                const DVM_NamespacePlan *source,
                                const DVM_NamespacePlan *target,
                                DVM_Anchor *anchor,
                                const char *component);


/* EN/RU: Make a fresh group rooted at selected directory; owned result or NULL.
 * Создать группу с root в выбранном каталоге; своя ссылка либо NULL. */
DVM_Namespace *dvm_namespace_rebase(DVM_ResourcePlan *cwd);
#endif
