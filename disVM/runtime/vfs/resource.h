/* Copyright (c) 2026 bhgv
 * EN: Refcounted directory resources and owner-local I/O plans, never managed roots.
 * RU: Refcounted directory resources и owner-local I/O plans; не managed roots.
 */
#ifndef DVM_VFS_RESOURCE_H
#define DVM_VFS_RESOURCE_H
#include "namespace.h"
#include "provider.h"
#include "launch/origin.h"
#include "anchor_port.h"
#include "disvm/native_cleanup.h"
typedef struct DVM_Anchor DVM_Anchor; /* Native capability with independently retained users. */


/* EN: Stable caller storage until close; never copy a live cleanup record.
 * RU: Стабильная caller storage до close; не копировать живой cleanup record. */
typedef struct DVM_ResourcePlan
{
    struct DVM_ResourcePlan *layers; /* Owned lower choices captured at the same revision. */
    int no_native_fallback; /* Refuse implicit host paths when no granted provider/anchor remains. */
    int absence_only; /* Launch default: lower lookup only on NOT_FOUND; explicit AUTORWX overrides. */
    DVM_Origin origin; /* Copied selected source association, no authority. */
    unsigned policy; /* Captured union-wide OR of explicit extension flags. */
    unsigned requested_access; /* Operation intent; zero for ordinary metadata lookup. */
    unsigned mount_flags; /* Create permission for this selected mount branch. */
    DVM_Namespace *group; /* Borrowed while plan lives; cursor retains independently. */
    uint64_t head_id; /* Retained mount-head identity for directory union enumeration. */
    uint64_t layer_id; /* Exact selected binding identity; zero for unmounted path. */
    int directory; /* Kind observed during branch selection; avoids an extra metadata probe before open. */
    int selected; /* Once selected, open must not fall through on permission errors. */
    int mount_root; /* Exact mounted root, including a single branch. */
    int union_root; /* Exact multi-layer directory root; enables merged enumeration. */
    DVM_NamespacePlan names;          /* Frozen virtual/display names and group revision. */
    char relative[DVM_NS_PATH_LIMIT]; /* Selected path relative to anchor, or empty for native fallback. */
    DVM_Provider *provider; /* Owned provider selection, alternative to native anchor. */
    DVM_Anchor *anchor;               /* Owned selection retained under the group lock. */
    DVM_Provider *created_provider; /* Owned staged cwd provider until commit/cleanup. */
    char created_component[DVM_NS_PATH_LIMIT]; /* Staged stable provider route or captured dot. */
    DVM_Anchor *created;              /* Owned new directory until commit retains it. */
    DVM_NativeCleanup cleanup;        /* Current native owner's unwind registration. */
} DVM_ResourcePlan;


/* EN/RU: Open owned native directory, possibly relative to borrowed parent;
 * return NULL on error. Открыть свой каталог от parent либо native path;
 * NULL при ошибке. Native-only blocking; no VM access. */
DVM_Anchor *dvm_anchor_open(DVM_Anchor *parent, const char *path);


/* EN/RU: Clone an owned confined capability from retained directory; owned/NULL.
 * Клонировать confined capability из retained directory; owned/NULL, native only. */
DVM_Anchor *dvm_anchor_confine(DVM_Anchor *parent);


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


/* EN/RU: Select an implementation for a typed module call from the caller's namespace/import base.
 * local_role is the trusted caller-relative TERMINAL/PROVIDER class, never a permission or wire claim.
 * Prefer visible candidates of that class, then remaining candidates, preserving each class's stack order.
 * Longest explicit target and retained cwd/import anchor rules still apply; namespace/data plans stay unchanged.
 * Return1 owned plan/0 failure; no provider I/O. Invalid input clears non-NULL empty output; do not overwrite live plans.
 * Выбрать реализацию typed module call из namespace/import base caller; local_role — trusted класс своей стороны.
 * Сначала видимые candidates своего класса, затем остальные; порядок внутри групп сохраняется.
 * Longest target и retained import anchor действуют прежним образом; namespace/data plans не меняются.
 * Вернуть1 owned plan/0 failure, без provider I/O; invalid очищает non-NULL empty output, live plan не перезаписывать.
 */
int dvm_resource_plan_call(DVM_Namespace *group, const char *path, unsigned local_role, DVM_ResourcePlan *plan);


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


/* EN/RU: Commit borrowed plan.created or created_provider/component as cwd; 1 success, 0 stale, -1 error.
 * Применить staged anchor/provider как cwd; 1 успех, 0 stale, -1 ошибка. Retains refs; no I/O. */
int dvm_namespace_chdir_resource(DVM_Namespace *group, DVM_ResourcePlan *plan);


/* EN/RU: Acquire a single directory capability in borrowed plan; one/zero.
 * Provider metadata/search/capture may block; no namespace lock or managed access.
 * Own staged resource in plan until close. Union provider cwd remains refused.
 * Захватить каталог в borrowed plan; один/ноль, native blocking без namespace lock.
 * Staged resource принадлежит plan до close; provider union cwd отклоняется. */
int dvm_resource_directory_acquire(DVM_ResourcePlan *plan);


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

/* EN/RU: Trusted host mount at an exact virtual target/revision; retain provider
 * on success. label is an absolute diagnostic source name, never a host lookup.
 * 1 success, 0 stale, -1 invalid/OOM. No callbacks or I/O under namespace mutex.
 * Host mount в exact virtual target/revision; при успехе удержать provider.
 * label — absolute diagnostic name, не host lookup. 1/0/-1: успех/stale/ошибка. */
int dvm_namespace_mount_provider(DVM_Namespace *group, const DVM_NamespacePlan *target,
                                 DVM_Provider *provider, const char *label);

/* EN: Same mount operation, optionally recording the exact installed layer in id.
 * RU: Та же установка провайдера с необязательной выдачей точного слоя в id. */
int dvm_namespace_mount_provider_id(DVM_Namespace *group, const DVM_NamespacePlan *target,
    DVM_Provider *provider, const char *label, uint64_t *id);


/* EN/RU: Bind retained provider-relative source at validated target/revision.
 * Caller checked resource kinds; source/target must share one group revision.
 * Retain factory on success; 1/0/-1 success/stale/invalid or OOM. No I/O.
 * Связать retained provider-relative source с проверенным target/revision.
 * Caller проверил kinds; одна revision. Удержать factory при успехе; 1/0/-1. */
int dvm_namespace_bind_provider(DVM_Namespace *group, const DVM_ResourcePlan *source,
                                const DVM_NamespacePlan *target);

/* EN/RU: Commit validated source/target layers with Inferno order/create flags.
 * Native source/target directories have acquired created anchors. No I/O.
 * Применить проверенные source/target со flags; native directory anchors уже
 * получены. 1/0/-1 success/stale/error; provider identity contract applies. */
int dvm_namespace_bind_layers(DVM_Namespace *group, DVM_ResourcePlan *source,
                              DVM_ResourcePlan *target, unsigned flags, int directory, const char *component);


/* EN/RU: Remove exact captured layer, or all layers for id=0; revision guarded.
 * Удалить captured layer либо все при id=0; проверять revision. 1/0/-1. */
int dvm_namespace_unmount_layer(DVM_Namespace *group, const DVM_NamespacePlan *target, uint64_t id);


/* EN/RU: Native-only selection/open helpers; no managed callbacks or namespace locks.
 * Native-only selection/open; без managed callbacks и namespace locks. */
FVM_Result dvm_union_info(DVM_ResourcePlan *plan, FVM_FileInfo *output);


/* EN/RU: Select existing entry, or first MCREATE branch for a missing create.
 * Выбрать existing entry либо первую MCREATE ветвь для missing create. */
FVM_Result dvm_union_select(DVM_ResourcePlan *plan, int creating);


/* EN/RU: Select first effectively permitted branch; create checks parent w+x.
 * Выбрать первую разрешённую ветвь; create проверяет parent w+x. Native-only;
 * borrowed plan, access intent from plan.requested_access; no effects/retry. */
FVM_Result dvm_union_auto_select(DVM_ResourcePlan *plan, int creating);


/* EN/RU: Swap selected resource into root without moving cleanup/list ownership.
 * Перенести selected resource в root без переноса cleanup/list ownership. */
void dvm_union_choose(DVM_ResourcePlan *root, DVM_ResourcePlan *choice);


/* EN/RU: Open merged directory into empty owned handle; input plans stay borrowed.
 * Открыть merged directory в пустой handle; input plans заимствованы. */
FVM_Result dvm_union_directory(DVM_ResourcePlan *plan, DVM_VfsHandle *output);

/* EN/RU: Query one branch only, without selecting lower layers; native-only.
 * Проверить одну ветвь без выбора нижних слоёв; native-only. */
FVM_Result dvm_resource_raw_info(DVM_ResourcePlan *plan, FVM_FileInfo *output);


/* EN/RU: Open owned directory cursor projecting exact child bindings beside existing entries.
 * Borrow plan/group through setup; native I/O, no guest execution. Original directory pins remain.
 * Открыть свой directory cursor с child bindings рядом с existing entries; plan borrowed, native I/O. */
FVM_Result dvm_namespace_directory(DVM_ResourcePlan *plan, DVM_VfsHandle *out);
#endif
