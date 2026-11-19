/* Copyright (c) 2026 bhgv
 * [GC:] EN: Provider-local per-Modlink namespace ownership; no per-machine cache or foreign heap pointers.
 * RU: Provider-local владение namespace каждым Modlink; без per-machine cache и чужих heap pointers.
 */
#ifndef DVM_MODULE_ENVIRONMENT_H
#define DVM_MODULE_ENVIRONMENT_H
#include "../vfs/namespace.h"
struct Modlink; /* Exact locally rooted module instance, never sent over Styx. */
struct Prog; /* Permanent runtime owner, borrowed only by that owner. */
typedef struct DVM_ModuleOverride DVM_ModuleOverride; /* Process-owned detached namespace variants. */


/* [GC:] EN/RU: Retain prepared live group on a private rooted local link before publication;1/0 invalid/OOM/duplicate.
 * No managed allocation, provider I/O or authority inference. Caller retains both inputs through return.
 * Each accepted remote instance supplies its own prepared owner-first group; no machine-keyed reuse.
 * Удержать prepared live group в private rooted local link до publication;1/0 invalid/OOM/duplicate.
 * Без managed allocation/provider I/O. Каждый remote instance получает собственную prepared group.
 */
int dvm_module_environment_bind(struct Modlink *link, DVM_Namespace *group);


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


/* EN/RU: Consume process-local overrides after its execution/borrowers retire; NULL process allowed, no managed access.
 * Освободить process-local overrides после execution/borrowers; NULL допустим, без managed access.
 */
void dvm_module_environment_release(struct Prog *process);
#endif
