/* Copyright (c) 2026 bhgv
 * [GC:] EN: Provider-local per-Modlink namespace ownership; no per-machine cache or foreign heap pointers.
 * RU: Provider-local владение namespace каждым Modlink; без per-machine cache и чужих heap pointers.
 */
#ifndef DVM_MODULE_ENVIRONMENT_H
#define DVM_MODULE_ENVIRONMENT_H
#include "../vfs/namespace.h"
struct Modlink; /* Exact locally rooted module instance, never sent over Styx. */
struct DVM_LaunchContext; /* Immutable owner-side label/arguments/contract, never a foreign heap object. */
struct Prog; /* Permanent runtime owner, borrowed only by that owner. */
typedef struct DVM_ModuleOverride DVM_ModuleOverride; /* Process-owned detached namespace variants. */


/* [GC:] EN/RU: Retain prepared live group on a private rooted local link before publication;1/0 invalid/OOM/duplicate.
 * No managed allocation, provider I/O or authority inference. Caller retains both inputs through return.
 * Each accepted remote instance supplies its own prepared owner-first group; no machine-keyed reuse.
 * Удержать prepared live group в private rooted local link до publication;1/0 invalid/OOM/duplicate.
 * Без managed allocation/provider I/O. Каждый remote instance получает собственную prepared group.
 */
int dvm_module_environment_bind(struct Modlink *link, DVM_Namespace *group);


/* [GC:] EN/RU: Retain prepared group and optional immutable native launch context on a private rooted link;1/0.
 * Same lifetime/publication rules as bind; context borrowed through return, independently retained on success.
 * No managed allocation/provider I/O; NULL context preserves caller-context inheritance.
 * Удержать prepared group и optional native launch context в private rooted link;1/0.
 * Context borrowed до return, retained при success; NULL сохраняет наследование caller context.
 */
int dvm_module_environment_bind_context(struct Modlink *link, DVM_Namespace *group, struct DVM_LaunchContext *context);


/* [GC:] EN/RU: Borrow nearest module's launch metadata, otherwise this ACTIVE permanent owner's process context; NULL if absent.
 * Search live rooted frames, including builtins/helpers. No allocation, I/O or ownership transfer.
 * Заимствовать launch metadata ближайшего module либо process context своего ACTIVE owner; NULL если нет.
 * Поиск по live rooted frames, без allocation/I/O/ownership transfer.
 */
struct DVM_LaunchContext *dvm_module_launch_current(struct Prog *process);


/* [GC:] EN/RU: Detach final link's native environment and queue its preallocated closer job; no I/O/allocation.
 * Called only by hosted freemodlink after all managed owners/readers retire; NULL/absent harmless.
 * Отсоединить native environment final link, поставить preallocated closer job; без I/O/allocation.
 */
void dvm_module_environment_retire(struct Modlink *link);


/* [GC:] EN/RU: Borrow nearest bound module's group through current owner's rooted call frames, NULL if none.
 * Process-local pctl overrides win for that exact immutable binding identity. ACTIVE owner only; no I/O/allocation.
 * Заимствовать group ближайшего bound module через rooted frames current owner; NULL если нет.
 * Process-local pctl override выбирается по exact binding identity. Только ACTIVE owner.
 */
DVM_Namespace *dvm_module_environment_current(struct Prog *process);


/* [GC:] EN/RU: Reserve current module's pctl slot before any external group-change notifications;1/0 OOM.
 * Empty slots grant nothing and retire with process. ACTIVE owner only, native allocation without I/O.
 * Зарезервировать pctl slot current module до external notifications;1/0 OOM. Empty slot не даёт authority.
 */
int dvm_module_environment_reserve(struct Prog *process);


/* [GC:] EN/RU: Consume replacement only on success, replacing current module override or ordinary process group.
 * Caller exclusively owns replacement;1/0 OOM, old native owner queued outside registry lock.
 * ACTIVE permanent owner only; FD table, terminal grant and other module instances stay untouched.
 * Принять replacement лишь при успехе; заменить module override/process group.1/0 OOM, старый owner на closer.
 */
int dvm_module_environment_replace(struct Prog *process, DVM_Namespace *replacement);


/* [GC:] EN/RU: Retain the spawning owner's effective bound-module namespace in a fresh unpublished child; return 1 on success.
 * Borrow parent/child through return; parent must be the ACTIVE owner for module-frame inspection.
 * Retain the current group rather than copy it; later FORKNS detaches only the process that requests it.
 * NULL/noncurrent constructor parent needs no module-frame selection; ordinary base-group inheritance stays in the constructor.
 * Child must have an empty override list. Failure publishes no override; private process cleanup releases other staged owners.
 * Return 0 for invalid child state, allocation failure or reference overflow. No blocking or managed allocation.
 * RU: Удержать выбранное окружение модуля родителя в ещё не опубликованном ребёнке; вернуть 1 при успехе.
 * Родитель и ребёнок заимствуются до возврата; читать кадры можно только своего активного родителя.
 * Группа разделяется до FORKNS. Список переопределений ребёнка должен быть пустым.
 * NULL/чужой родитель не требует записи выбора модуля; основную группу наследует конструктор.
 * При ошибке состояния, выделения памяти или переполнении ссылки вернуть 0 без публикации записи.
 * Без блокировки и выделения управляемых объектов; откат остальных ссылок выполняет конструктор.
 */
int dvm_module_environment_inherit(struct Prog *child, struct Prog *parent);


/* EN/RU: Consume process-local overrides after its execution/borrowers retire; NULL process allowed, no managed access.
 * Освободить process-local overrides после execution/borrowers; NULL допустим, без managed access.
 */
void dvm_module_environment_release(struct Prog *process);
#endif
